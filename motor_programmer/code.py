from adafruit_macropad import MacroPad
import time
import crc
import struct
from busio import UART
import microcontroller

print()

macropad = MacroPad()
text_lines = macropad.display_text(title="MacroPad Info")
pixels = macropad.pixels


class ChecksumError(ValueError):
    pass


class MessageLengthError(ValueError):
    pass


class Parameters:
    def __init__(self):
        # Pinout:
        # https://cdn-learn.adafruit.com/assets/assets/000/104/022/original/adafruit_products_Adafruit_MacroPad_RP2040_Pinout.png?1629726427
        self._uart = UART(
            tx=microcontroller.pin.GPIO20,
            rx=microcontroller.pin.GPIO21,
            baudrate=38_400,
        )

    def __len__(self):
        return 304

    def __getitem__(self, key):
        read_command = struct.pack(
            ">BBHH",
            # Address
            63,
            # Read holding registers
            3,
            # Offset
            key,
            # Count
            1,
        )
        self._send(read_command)
        address, function, byte_count, value = self._recv(">BBBH")
        print("Response: ", (address, function, byte_count, value))
        return value

    def __setitem__(self, key, value):
        write_command = struct.pack(
            ">BBHH",
            # Address
            63,
            # Write holding register
            6,
            # Offset
            key,
            # Value
            value,
        )
        self._send(write_command)
        (write_response,) = self._recv(">6s")
        if write_response != write_command:
            raise ValueError(
                f"Expected response: {list(write_command)} Actual response: {list(write_response)}"
            )

    def _send(self, data):
        data = data + crc.checksum(data).to_bytes(2, "little")
        print("Sending: ", list(data))
        self._uart.write(data)

    def _recv(self, format):
        expected_size = struct.calcsize(format) + 2
        data = self._uart.read(expected_size)
        if data is None:
            raise MessageLengthError("No data; timeout?")
        if (actual_size := len(data)) != expected_size:
            raise MessageLengthError(
                f"{expected_size=} {actual_size=} data={list(data)}"
            )
        print("Received: ", list(data))

        payload = data[:-2]
        expected_checksum = tuple(data[-2:])
        actual_checksum = tuple(crc.checksum(payload).to_bytes(2, "little"))
        if actual_checksum != expected_checksum:
            raise ChecksumError(f"{expected_checksum=} {actual_checksum=}")

        return struct.unpack(format, payload)


params = Parameters()
#params[4] = 301
print("param 4: ", params[4])


class LatencyLogger:
    def __init__(self, label):
        self._label = label

    def __enter__(self):
        self._start_ns = time.monotonic_ns()

    def __exit__(self, *args, **kwargs):
        now_ns = time.monotonic_ns()
        duration_ms = (now_ns - self._start_ns) / 1e6
        print(f"{self._label}: {duration_ms:7.3f}ms")


while True:
    event = macropad.keys.events.get()
    if event:
        key_number = event.key_number
        if event.pressed:
            pixels[key_number] = (255, 255, 255)
        else:
            pixels[key_number] = 0
