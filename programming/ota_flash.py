import argparse
import base64
import time

import click
import serial


READY_SIGNAL = b"<Ready for data>"
ESCAPE_SEQUENCE = [b'\4', b'\4', b'\4', b'!']

def main(port, file, baud=115200, chunk_size=64):
    print(
        "Flashing {file} to device at {port} ({baud})...".format(
            file=file,
            port=port,
            baud=baud,
        )
    )

    with serial.Serial(port, baud) as ser:
        ser.write(b'delay_bt_timeout 3600\n');
        for byte in ESCAPE_SEQUENCE:
            ser.write(byte)
            time.sleep(0.75)

        ser.write(b'flash_esp32\n')

        output = bytearray()

        while True:
            line = ser.readline().strip()
            if(line != READY_SIGNAL):
                print(line.strip().decode('utf8'))
            else:
                print("Sending data...")
                with open(file, 'rb') as inf:
                    with click.progressbar(inf.read()) as data:
                        for byte in data:
                            output += byte.to_bytes(1, byteorder='big')
                            if(len(output) >= chunk_size):
                                transmit(ser, output)
                                output = bytearray()
                        transmit(ser, output)
                        output = bytearray()
                print("Data transmission completed.")
                break

        while True:
            line = ser.readline().strip()
            if not line:
                break
            print(line.strip().decode('utf8'))


def transmit(ser, data):
    ser.write(base64.b64encode(data))
    ser.write(b'\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('port', type=str)
    parser.add_argument('file', type=str)
    parser.add_argument('--baud', type=int, default=115200)
    parser.add_argument('--chunk-size', type=int, default=64)

    args = parser.parse_args()

    main(args.port, args.file, baud=args.baud, chunk_size=args.chunk_size)
