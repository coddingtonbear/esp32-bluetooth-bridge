#include "Arduino.h"
#include "BluetoothSerial.h"

#include "esp_bt.h"

#include "multiserial.h"
#include "main.h"
#include "commands.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

char BT_CTRL_ESCAPE_SEQUENCE[] = {'\4', '\4', '\4', '!'};
uint8_t BT_CTRL_ESCAPE_SEQUENCE_LENGTH = sizeof(BT_CTRL_ESCAPE_SEQUENCE)/sizeof(BT_CTRL_ESCAPE_SEQUENCE[0]);

BluetoothSerial SerialBT;
unsigned long lastSend = 0;
bool connected = false;
String sendBuffer;
String commandBuffer;

int8_t escapeSequencePos = 0;
unsigned long lastEscapeSequenceChar = 0;

bool bridgeInit = false;
bool ucTx = false;

HardwareSerial UCSerial(1);
MultiSerial CmdSerial;

void setup() {
    pinMode(BT_KEY, INPUT_PULLDOWN);
    pinMode(PIN_CONNECTED, OUTPUT);
    digitalWrite(PIN_CONNECTED, LOW);
    pinMode(UC_NRST, INPUT);

    esp_bredr_tx_power_set(ESP_PWR_LVL_N0, ESP_PWR_LVL_P9);

    SerialBT.begin(BT_NAME);
    Serial.begin(115200);
    UCSerial.begin(230400, SERIAL_8E1, UC_RX, UC_TX);
    UCSerial.setRxBufferSize(1024);

    CmdSerial.addInterface(&Serial);
    CmdSerial.addInterface(&SerialBT);
    CmdSerial.addInterface(&UCSerial);

    sendBuffer.reserve(MAX_SEND_BUFFER);
    commandBuffer.reserve(MAX_CMD_BUFFER);

    setupCommands();

    while(CmdSerial.available()) {
        CmdSerial.read();
    }
    CmdSerial.disableInterface(&SerialBT);
    CmdSerial.disableInterface(&UCSerial);

    Serial.print("<Serial Bridge Ready: ");
    Serial.print(BT_NAME);
    Serial.println(">");
    commandPrompt();
}

void sendBufferNow() {
    int sentBytes = 0;
    if(connected) {
        if(sendBuffer.length() > 0) {
            while(sentBytes < sendBuffer.length()) {
                sentBytes += SerialBT.write(
                    &(((const uint8_t*)sendBuffer.c_str())[sentBytes]),
                    sendBuffer.length() - sentBytes
                );
            }
        }
    }
    sendBuffer = "";
    lastSend = millis();
}

void loop() {
    commandLoop();

    bool _connected = SerialBT.hasClient();

    if(connected != _connected) {
        connected = _connected;

        if(connected) {
            Serial.println("<Client Connected>");
        } else {
            Serial.println("<Client Disconnected>");
        }
        digitalWrite(PIN_CONNECTED, connected);
    }

    if(UCSerial.available()) {
        int read = UCSerial.read();

        if(read != -1) {
            if(digitalRead(BT_KEY) == HIGH) {
                // The uC is trying to send us a command; let's process
                // it as such.
                commandByte(read);
            } else {
                if(monitorBridgeEnabled()) {
                    if(!ucTx || bridgeInit == false) {
                        Serial.println();
                        Serial.print("UC> ");
                        ucTx = true;
                        bridgeInit = true;
                    }
                    Serial.print((char)read, HEX);
                    Serial.print(' ');
                }

                sendBuffer += (char)read;
                if(
                    ((char)read == '\n') 
                    || sendBuffer.length() >= (MAX_SEND_BUFFER - 1)
                ) {
                    sendBufferNow();
                }
            }
        }
    } else if (millis() - lastSend > MAX_SEND_WAIT) {
        sendBufferNow();
    }
    if(!escapeIsEnabled()) {
        if(SerialBT.available()) {
            int read = SerialBT.read();

            if(read != -1) {
                if(monitorBridgeEnabled()) {
                    if(ucTx || bridgeInit == false) {
                        Serial.println();
                        Serial.print("BT> ");
                        ucTx = false;
                        bridgeInit = true;
                    }
                    Serial.print((char)read, HEX);
                    Serial.print(' ');
                }
                UCSerial.write((char)read);
                if(
                    read == BT_CTRL_ESCAPE_SEQUENCE[escapeSequencePos]
                    && (
                        millis() > (
                            lastEscapeSequenceChar + BT_CTRL_ESCAPE_SEQUENCE_INTERCHARACTER_DELAY
                        )
                    )
                ) {
                    lastEscapeSequenceChar = millis();
                    escapeSequencePos++;
                } else {
                    escapeSequencePos = 0;
                }
                if(escapeSequencePos == BT_CTRL_ESCAPE_SEQUENCE_LENGTH) {
                    enableEscape();
                }
            }
        }
    }
}
