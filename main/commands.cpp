#include "SerialCommand.h"
#include "commands.h"
#include "libb64/cdecode.h"
#include "esp_ota_ops.h"
#include "commands.h"
#include "main.h"

SerialCommand commands(&CmdSerial);

bool connectedHigh = false;
bool monitorEnabled = true;
bool escapeEnabled = false;

void setupCommands() {
    commands.addCommand("flash_esp32", flashEsp32);
    commands.addCommand("flash_uc", flashUC);
    commands.addCommand("reset_uc", resetUC);
    commands.addCommand("connected", connected);
    commands.addCommand("monitor", monitorBridge);
    commands.addCommand("nrst", setRst);
    commands.addCommand("unescape", unescape);
    commands.setDefaultHandler(unrecognized);
}

void unescape() {
    CmdSerial.disableInterface(&SerialBT);
    escapeEnabled = false;
}

void enableEscape() {
    CmdSerial.enableInterface(&SerialBT);
    CmdSerial.println("<escape sequence received>");
    escapeEnabled = true;
}

void commandPrompt() {
    commands.prompt();
}

void commandLoop() {
    commands.readSerial();
}

void commandByte(char inChar) {
    commands.readChar(inChar);
}

void unrecognized(const char *cmd) {
    CmdSerial.print("<Unknown command: ");
    CmdSerial.print(cmd);
    CmdSerial.println(">");
}

bool monitorBridgeEnabled() {
    return monitorEnabled;
}

bool escapeIsEnabled() {
    return escapeEnabled;
}

void setRst() {
    char* state = commands.next();

    if(state == NULL) {
        pinMode(UC_NRST, INPUT);
    } else {
        bool nrstState = atoi(state);

        pinMode(UC_NRST, OUTPUT);
        if(nrstState) {
            digitalWrite(UC_NRST, HIGH);
        } else {
            digitalWrite(UC_NRST, LOW);
        }
    }
}

void monitorBridge() {
    char* state = commands.next();

    if(state == NULL) {
        Serial.println(monitorEnabled ? "1": "0");
        return;
    }
    monitorEnabled = atoi(state);
}

void connected() {
    char* state = commands.next();

    if(state == NULL) {
        Serial.println(connectedHigh ? "1": "0");
        return;
    }

    int target = atoi(state);
    if(target) {
        connectedHigh = true;
        digitalWrite(PIN_CONNECTED, HIGH);
    } else {
        connectedHigh = false;
        digitalWrite(PIN_CONNECTED, LOW);
    }
}

void resetUC() {
    pinMode(UC_NRST, OUTPUT);
    digitalWrite(UC_NRST, LOW);
    delay(500);
    digitalWrite(UC_NRST, HIGH);
    pinMode(UC_NRST, INPUT);
}

void flashUC() {
    digitalWrite(PIN_CONNECTED, HIGH);
    delay(250);
    resetUC();
    delay(500);
    digitalWrite(PIN_CONNECTED, LOW);
}

void flashEsp32() {
    CmdSerial.println("<OTA flash>");
    CmdSerial.flush();
    esp_err_t err;
    esp_ota_handle_t update_handle = 0;

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *update_partition = NULL;

    unsigned long last_data = millis();

    CmdSerial.disableInterface(&UCSerial);
    if(configured != running) {
        CmdSerial.println("Warning: OTA boot partition does not match running partition.");
    }

    char otaReadData[OTA_BUFFER_SIZE + 1] = {0};
    char otaWriteData[OTA_BUFFER_SIZE + 1] = {0};
    uint16_t bufferLength = 0;
    uint16_t bytesDecoded = 0;

    update_partition = esp_ota_get_next_update_partition(NULL);
    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        CmdSerial.print("Error beginning OTA update: ");
        CmdSerial.println(esp_err_to_name(err));
        goto cleanUp;
    }
    CmdSerial.println("<Ready for data>");
    SerialBT.flush();
    // pad last_data slightly to allow for a delayed start
    last_data = millis() + 5000;

    while(true) {
        while(!SerialBT.available()) {
            if(millis() > (last_data + 1000)) {
                CmdSerial.println("<transmission ended>");
                goto updateReady;
            }
        }

        uint bytesRead = SerialBT.readBytesUntil('\n', otaReadData, OTA_BUFFER_SIZE);
        last_data = millis();

        if(bytesRead == 0) {
            continue;
        }

        base64_decodestate decodeState;
        base64_init_decodestate(&decodeState);
        bufferLength = base64_decode_block(otaReadData, bytesRead, otaWriteData, &decodeState);
        bytesDecoded += bufferLength;

        err = esp_ota_write(update_handle, otaWriteData, bufferLength);
        if(err != ESP_OK) {
            CmdSerial.print("Could not write data: ");
            CmdSerial.println(esp_err_to_name(err));
            goto cleanUp;
        }
    }

    updateReady:
        CmdSerial.print(bytesDecoded);
        CmdSerial.println(" bytes written");
        err = esp_ota_end(update_handle);
        if(err != ESP_OK) {
            CmdSerial.print("Could not complete update: ");
            CmdSerial.println(esp_err_to_name(err));
            goto cleanUp;
        }

        err = esp_ota_set_boot_partition(update_partition);
        if(err != ESP_OK) {
            CmdSerial.print("Could not set boot partition: ");
            CmdSerial.println(esp_err_to_name(err));
            goto cleanUp;
        }

        CmdSerial.println("<completed: success>");
        CmdSerial.flush();
        delay(5000);
        esp_restart();

    cleanUp:
        CmdSerial.println("<completed: failure>");
        CmdSerial.flush();
        delay(5000);
        esp_restart();
}
