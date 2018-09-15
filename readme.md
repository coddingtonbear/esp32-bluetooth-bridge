# ESP32 Bluetooth Bridge (Replaces HC-05)

Do you have a project where you'd like it to be possible for you to connect
to (and potentially reprogram) another microcontroller over Bluetooth, but
also want to have the flexibility to add new Bluetooth, BLE, or WIFI
features?  I previously used the HC-05 bluetooth module for providing Bluetooth
tty access for interactions and programming; the ESP32, though, is only
slightly more expensive and provides many features that the HC-05
cannot offer -- including that it itself can be programmed over-the-air to
add your own features and functionality very easily.

This repository contains a simple Bluetooth-to-UART bridge roughly
mirroring how the HC-05 behaved, but adds a handful of new features:

* An escape sequence allowing you to break out of your serial bridge
  to send commands to the wireless unit itself directly.
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

## Commands

### `flash_esp32`

This command begins an OTA flash of the ESP32 unit itself.  In general,
there is no need for you to run this command directly, instead use the
included python script in `programming/ota_flash.py` to flash the ESP32
unit over bluetooth.  See "Flashing the ESP32 Over-the-air" for details.

### `flash_uc`

This command is designed to reboot an STM32 microcontroller into its
serial bootloader by:

* Pulling its BOOT0 pin high (see `PIN_CONNECTED` in `main.h`)
* Pulling its nRST line low for 250ms (see `UC_NRST` in `main.h`)
* Pulling its nRST line high

At that point, the microcontroller will be ready to accept programming
over bluetooth.

### `reset_uc`

Briefly pulls the microcontroller's reset line low to cause it to restart.

### `boot0 [0|1]`

* When called without an argument: returns the current state of the BOOT0 pin
(i.e. `PIN_CONNECTED`).
* When called with an argument of `0`: Pulls BOOT0 (`PIN_CONNECTED`) low.
* When called with an argument of `1`: Pulls BOOT0 (`PIN_CONNECTED`) high.

`PIN_CONNECTED` is generally connected directly to the `BOOT0` pin of the
microcontroller to emulate the procedure historically used for using an
HC-05 unit to flash an STM32 microcontroller.  You'll notice that they
are used interchangably throughout this document and in the source, but do
know that you can adjust the state of this pin independently from its
default behavior of indicating whether a client is connected.

### `monitor [0|1]`

* When called without an argument: returns the current state of the serial
  monitor.
* When called with an argument of `0`: Turns serial monitoring off.
* When called with an argument of `1`: Turns serial monitoring on.

Note that this is probably only useful if you are issuing commands
to the ESP32 unit's UART1 instead of communicating over Bluetooth.

### `nrst [0|1]`

* When called without an argument: stops setting the state of the
  microcontroller's nRST pin by reconfiguring the corresponding ESP32
  pin as an input.
* When called with an argument of `0`: Pulls nRST (`UC_NRST`) low.
* When called with an argument of `1`: Pulls nRST (`UC_NRST`) high.

### `unescape`

Exits "escaped" mode if the device had previously recieved
the relevant escape sequence.  This is useful for allowing you to
re-enable pass-through functionality after issuing an escape sequence
to send your microcontroller a command.

## Building the firmware

First, make sure that you have installed the xtensa compiler
(`xtensa-esp32-elf-gcc`) and that it is available on your PATH.

Next, make sure you've cloned all necessary submodules:

```
git submodule update --init --recursive
```

After that, set the `IDF_PATH` environment variable
to point at your clone of the esp32 IDF (https://github.com/espressif/esp-idf) --
and make sure that you've checked out a commit that is compatible with
the version of arduino-esp32 in use.  If you're running this repository
as it is now, that commit should be `aaf12390`.

Now, just ask it to compile:

```
make
```

You're now ready to flash that firmware onto the device.  Note that if you
are flashing over the air, you should follow the procedure described below
under "Flashing the ESP32 Over-the-air" instead of following the usual
`make flash` procedure.

## Escape Sequence

*Default*: `CTRL+D`, `CTRL+D`, `CTRL+D`, `!`

The escape sequence keys must be pressed _at least_ 500ms apart from one
another (and, obviously, no other keys may be pressed between each of your
escape sequence bytes).  This might sound a little strange, but this
behavior exists as a way of making sure that the bytes transmitted as
part of your escape sequence aren't occurring naturally as part of
some other data you're intentionally transmitting to your microcontroller.

If you want to make adjustments to this behavior, see:

* `main.h`: `BT_CTRL_ESCAPE_SEQUENCE_INTERCHARACTER_DELAY` to adjust the
  minimum amount of time that must pass between each character of your
  escape sequence.
* `main.cpp`; `BT_CTRL_ESCAPE_SEQUENCE` to adjust the escape sequence itself.


## Flashing the ESP32 Over-the-air

It's possible to flash the ESP32 unit itself over blueotooth by
using the included python script (`programming/ota_flash.py`); to do that,
follow the instructions below.

Note that flashing is essentially 100% safe for an ESP32 module; the
device's built-in Over-the-air programming functionality is cleverly
designed and will not switch to the newly-programmed source unless it
passes a verification procedure.  If the flashing process fails for any
reason, the installed code will remain unchanged.

### Building the firmware

See "Building the firmware".

### Installing Dependencies

From your clone of this repository, run the following commands:

```
cd programming
virtualenv . --python=python3
source bin/activate
pip install -r requirements.txt
cd ..
```

### Flashing the firmware

From your clone of this repository, run the following commands:

```
cd programming
python ota_flash.py /path/to/bluetooth/device
```

By default, this will:

* Connect to the device you have specified (at which point, you will
  be connected via the pass-through to the connected microcontroller).
* Sends the "Escape Sequence" mentioned above to escape the pass-through.
* Issues the command `flash_esp32`.
* Waits for the ESP32 unit to be ready.
* Sends the new firmware stored in `../build/bridge.bin`.
* Prints any messages received from the ESP32 unit during this process.

At the end of this process, you should see one of the following messages:

* `<completed: success>`: If the flashing process completed successfully.
* `<completed: failure>`: If the flashing process failed for some reason.
  Consult the other displayed messages to determine a potential cause
  for this flashing fialure.  Note that failures are completely safe, and
  you can try re-flashing again as soon as you'd like.

See `python ota_flash.py --help` for other options.
