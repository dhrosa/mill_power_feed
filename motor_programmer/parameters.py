import crc
import struct
from busio import UART
import microcontroller


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
        self._check_key(key)
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
        self._check_key(key)
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

    def _check_key(self, key):
        if int(key) != key:
            raise TypeError(f"Non-integer key: {key}")
        key = int(key)
        if key >= 0 and key < len(self):
            return
        raise IndexError(f"Key ({key}) outside of range [0, {len(self)})")

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
