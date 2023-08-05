import crc
import struct
from page import Page


class ChecksumError(ValueError):
    pass


class MessageLengthError(ValueError):
    pass


class Parameters:
    def __init__(self, uart):
        self._uart = uart
        self._cache = {}
        boundaries = (0, 65, 95, 125, 175, 215, len(self))
        self._pages = []
        for i in range(len(boundaries) - 1):
            number = i + 1
            start = boundaries[i]
            stop = boundaries[i + 1] - 1
            self._pages.append(Page(self, number, start, stop))

    @property
    def pages(self):
        return self._pages

    def __len__(self):
        return 304

    def __getitem__(self, key):
        key = self._check_key(key)
        if key in self._cache:
            return self._cache[key]
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
        self._cache[key] = value
        return value

    def __setitem__(self, key, value):
        key = self._check_key(key)
        del self._cache[key]
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
        return int(key) % len(self)

    def _send(self, data):
        data = data + crc.checksum_bytes(data)
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

        payload = data[:-2]
        expected_checksum = data[-2:]
        actual_checksum = crc.checksum_bytes(payload)
        if actual_checksum != expected_checksum:
            raise ChecksumError(
                f"{expected_checksum=} {actual_checksum=} data={list(data)}"
            )

        return struct.unpack(format, payload)
