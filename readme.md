# ESP32 Bluetooth Bridge (Replaces HM-10)

I used to use the HM-10 bluetooth module for providing wireless
tty access for interactions and programming; the ESP32, though, is only
slightly more expensive and provides many features that the HM-10
cannot offer -- including that it can be programmed.

This repository contains a simple Bluetooth-to-UART bridge roughly
mirroring how the HM-10 behaved, but adds a handful of new features:

* An escape sequence -- by default `CTRL+D`, `CTRL+D`, `CTRL+D`, `!`,
  each pressed at _least_ 500ms apart from one another to help prevent
  false positives when binary data is being sent -- allowing you to
  access serial commands on the ESP32 device itself.  I've used this
  as my escape hatch for providing a way for me to program my main
  microcontroller.
* The ability to accept commands including one allowing you to monitor
  the bluetooth bridge via the ESP32's UART1.
* Configurable pin connections for:
    * Indicating when a client is connected over bluetooth (`PIN_CONNECTED`)
    * Allowing the bridged microcontroller to send commands directly to the
      ESP32 (`BT_KEY`).
    * Connecting to the Microcontroller's reset line for debugging,
      flashing, and troubleshooting.
    * RX/TX output pins for connecting to the microcontroller.
* A variety of (partially STM32-specific) commands and the ability
  for you to add your own very easily.

Hopefully somebody other than me finds this useful!
