#pragma once

#include "Arduino.h"
#include "BluetoothSerial.h"
#include "multiserial.h"

// This is the name that this device will appear under during discovery
#define BT_NAME "esp32-bridge"

// At least this number of ms must elapse between each
// character of the escape sequence for it to be counted; this
// is done to prevent legitimate occurrences of the escape sequence
// occurring in binary causing us to enter the bridge mode.
#define BT_CTRL_ESCAPE_SEQUENCE_INTERCHARACTER_DELAY 500

// For communicating with the microcontroller, we will transmit and
// receive on the following pins.  Do not set these to match the pins
// used by defualt for the ESP32 unit's UART1 (IO35 and IO34).  I've also
// heard, but not confirmed, that the IO pin used for TX must be a higher
// IO number than the pin used for RX.  If you can confirm or deny this,
// I'd love to hear a definitive answer.
#define UC_TX 15
#define UC_RX 2

// This pin will be pulled HIGH when a client is connected over bluetooth.
#define PIN_CONNECTED 4

// If your microcontroller pulls this pin HIGH, it can send commands
// directly to the ESP32 unit
#define BT_KEY 34

// Connect this to your microcontoller's reset line to allow you to
// reset your microcontoller at-will.
#define UC_NRST 32

#define MAX_SEND_WAIT 50
#define MAX_CMD_BUFFER 128
#define MAX_SEND_BUFFER 128

void setup();
void loop();
void sendBufferNow();

extern MultiSerial CmdSerial;
extern HardwareSerial UCSerial;
extern BluetoothSerial SerialBT;
