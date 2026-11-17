"""Send one framework command or run the UART acceptance sequence."""
import argparse
import sys
import time


def packet(command, argument=0):
    return bytes((0xA5, command, argument, (0xA5 + command + argument) & 0xFF))


def collect(port, seconds):
    end = time.monotonic() + seconds
    received = bytearray()
    while time.monotonic() < end:
        received.extend(port.read(max(1, port.in_waiting)))
    text = received.decode("ascii", "replace")
    print(text, end="", flush=True)
    return text


def smoke(port):
    def check(payload, expected, seconds=0.3):
        port.write(payload)
        observed = collect(port, seconds).splitlines()
        # A peripheral fault can arrive while checking the console responses.
        replies = [line for line in observed
                   if line.startswith(("[ECHO]", "[FRAME]", "[UART]", "[CMD]"))]
        if replies != expected:
            raise RuntimeError(f"expected {expected!r}, got {replies!r}")

    check(packet(1, 17) + packet(1, 18), ["[ECHO][Ok] arg=17", "[ECHO][Ok] arg=18"])
    wrong = bytearray(packet(1))
    wrong[-1] ^= 1
    check(wrong + packet(1, 19), ["[FRAME][ChecksumError] receive", "[ECHO][Ok] arg=19"])
    check(packet(1)[:2], ["[FRAME][Timeout] partial frame"])
    check(packet(1, 20), ["[ECHO][Ok] arg=20"])
    print("UART acceptance sequence passed.")


def number(argument):
    value = int(argument, 0)
    if not 0 <= value <= 255:
        raise argparse.ArgumentTypeError("expected a byte")
    return value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", help="e.g. COM5")
    parser.add_argument("command", help="smoke or a numeric command, e.g. 0x01")
    parser.add_argument("argument", nargs="?", type=number, default=0)
    parser.add_argument("--seconds", type=float, default=1)
    args = parser.parse_args()
    import serial
    with serial.Serial(args.port, 115200, timeout=0.02) as port:
        port.reset_input_buffer()
        if args.command == "smoke":
            smoke(port)
        else:
            port.write(packet(number(args.command), args.argument))
            collect(port, args.seconds)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError) as error:
        sys.exit(str(error))
