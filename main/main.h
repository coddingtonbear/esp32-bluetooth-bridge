#pragma once

#include "Arduino.h"
#include "BluetoothSerial.h"
#include "multiserial.h"

#define MAX_SEND_WAIT 50
#define MAX_CMD_BUFFER 128
#define MAX_SEND_BUFFER 2048

#define BT_NAME "maxwell-remote"
// At least this number of ms must elapse between each
// character of the escape sequence for it to be counted; this
// is done to prevent legitimate occurrences of the escape sequence
// occurring in binary causing us to enter the bridge mode.
#define BT_CTRL_ESCAPE_SEQUENCE_INTERCHARACTER_DELAY 500

#define PIN_CONNECTED 4
#define UC_TX 15
#define UC_RX 2
#define BT_KEY 34
#define UC_NRST 32

void setup();
void loop();
void sendBufferNow();

extern MultiSerial CmdSerial;
extern HardwareSerial UCSerial;
extern BluetoothSerial SerialBT;
