import argparse
import base64
import time

import click
import serial


READY_SIGNAL = b"<Ready for data>"


class OtaFailed(Exception):
    pass


def main(
    port,
    file,
    baud=115200,
    chunk_size=700,
    escape_sequence=None,
    escape_sequence_interbyte_delay=0,
    pre_escape_commands=[],
):
    print(
        "Flashing {file} to device at {port} ({baud})...".format(
            file=file,
            port=port,
            baud=baud,
        )
    )

    with serial.Serial(port, baud, timeout=1) as ser:
        if pre_escape_commands:
            ser.write(b'\n')
            for command in pre_escape_commands:
                ser.write(command.encode('ascii'))
                ser.write(b'\n')

        if escape_sequence:
            for byte in escape_sequence:
                ser.write(byte)
                time.sleep(escape_sequence_interbyte_delay)
            ser.write(b'\n')

        ser.write(b'flash_esp32\n')

        while True:
            line = ser.readline().strip()
            if(line != READY_SIGNAL):
                print(line.strip().decode('utf8'))
            else:
                print("Sending data...")
                with open(file, 'rb') as inf:
                    try:
                        transmit_file(ser, inf, chunk_size)
                    except OtaFailed:
                        print_serial_responses(ser)
                        raise
                print("Data transmission completed.")
                print_serial_responses(ser)
                break


def transmit_file(ser, inf, chunk_size):
    output = bytearray()

    with click.progressbar(inf.read()) as data:
        for byte in data:
            output += byte.to_bytes(1, byteorder='big')
            if(len(output) >= chunk_size):
                transmit_chunk(ser, output)
                output = bytearray()

            if ser.in_waiting:
                line = ser.readline().strip()
                if line:
                    line = line.decode('ascii')
                    print(line)
                    if line.startswith(
                        "<Serial bridge ready"
                    ):
                        raise OtaFailed(
                            "Device unexpectedly restarted."
                        )
                    elif line.startswith(
                        "<completed: failure>"
                    ):
                        raise OtaFailed("Flash failed.")
                    elif line.startswith(
                        "<completed: success>"
                    ):
                        raise OtaFailed(
                            "Unexpected early completion."
                        )
        transmit_chunk(ser, output)


def transmit_chunk(ser, data):
    ser.write(base64.b64encode(data))
    ser.write(b'\n')


def print_serial_responses(ser, seconds=3):
    occurred = time.time()
    # Collect any serial messages that occur over
    # the next three seconds
    while(time.time() < occurred + seconds):
        if ser.in_waiting:
            line = ser.readline().strip()
            if line:
                print(line.decode('utf-8'))


def type_escape_sequence(data):
    escape_sequence_bytes = []

    for part in data.split(','):
        if part.startswith('0x'):
            escape_sequence_bytes.append(int(part, 16))
            continue

        escape_sequence_bytes.append(ord(part.encode('ascii')))

    return escape_sequence_bytes


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('port', type=str)
    parser.add_argument(
        '--file',
        type=str,
        help=(
            'ESP32 programming file to flash over-the-air to the device.  '
            'Note that this file is the file having the extension `.bin`, not '
            'the file having the extension `.elf`!.  This defaults to '
            '`../build/bridge.bin`.'
        ),
        default='../build/bridge.bin',
    )
    parser.add_argument('--baud', type=int, default=115200)
    parser.add_argument(
        '--escape-sequence',
        type=type_escape_sequence,
        help=(
            'Comma-separated bytes to transmit to escape the serial '
            'pass-through to the microcontroller.  This should match '
            '`BT_CTRL_ESCAPE_SEQUENCE` in `main.cpp`.  Non-printable '
            'characters can be specified as hexadecimal values prefixed '
            'with `0x`.  Defaults to `0x04,0x04,0x04,!`.'
        ),
        default=[b'\4', b'\4', b'\4', b'!']
    )
    parser.add_argument(
        '--escape-sequence-interbyte-delay',
        type=float,
        help=(
            'Amount of time (in seconds) to delay between each byte '
            'of the escape sequence.  Defaults to 0.75s.'
        ),
        default=0.75,
    )
    parser.add_argument(
        '--pre-escape-command',
        type=str,
        help=(
            'Commands to issue to the pass-through microcontroller prior '
            'to escaping the serial pass-through.  These are intended to '
            'be used as a way of preventing the pass-through microcontroller '
            'turning off the ESP32 unit during programming.  By default, '
            'this sends `delay_bt_timeout 600`.'
        ),
        default=[
            'delay_bt_timeout 600'
        ],
        action='append',
    )
    parser.add_argument(
        '--chunk-size',
        type=int,
        help=(
            'Maximum chunk size use for sending binary data.  Binary data '
            'is sent in Base64-encoded chunks that the ESP32 unit must then '
            'decode into their actual bytes.  The higher this value is, '
            'the faster the device may be flashed, but this number of bytes '
            'cannot after being Base64-encoded exceed the buffer size '
            'the ESP32 unit has reserved for this process (generally '
            '1024kB); on average Base64 encoding bytes results in a 40% '
            'increase in size.  This value defaults to 700 resulting in '
            'a transmitted byte count of about 980 bytes per chunk.'
        ),
        default=700,
    )

    args = parser.parse_args()

    main(
        args.port,
        args.file,
        baud=args.baud,
        chunk_size=args.chunk_size,
        escape_sequence=args.escape_sequence,
        escape_sequence_interbyte_delay=args.escape_sequence_interbyte_delay,
        pre_escape_commands=args.pre_escape_command,
    )
