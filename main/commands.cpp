#include "SerialCommand.h"

#include "commands.h"
#include "main.h"

SerialCommand commands(&CmdSerial);

bool boot0High = false;
bool monitorEnabled = false;
bool escapeEnabled = false;

void setupCommands() {
    commands.addCommand("flash_uc", flashUC);
    commands.addCommand("reset_uc", resetUC);
    commands.addCommand("boot0", boot0);
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
    Serial.print("Unknown command: ");
    Serial.println(cmd);
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

void boot0() {
    char* state = commands.next();

    if(state == NULL) {
        Serial.println(boot0High ? "1": "0");
        return;
    }

    int target = atoi(state);
    if(target) {
        boot0High = true;
        digitalWrite(PIN_CONNECTED, HIGH);
    } else {
        boot0High = false;
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
