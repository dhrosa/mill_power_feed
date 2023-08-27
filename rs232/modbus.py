import struct


class ChecksumError(ValueError):
    pass


class MessageLengthError(ValueError):
    pass


class Modbus:
    DEVICE_ADDRESS = 63

    def __init__(self, stream):
        self._stream = stream

    def _send(self, payload):
        self._stream.write(payload + crc.checksum_bytes(payload))

    def _recv(self, format):
        expected_size = struct.calcsize(format) + 2
        data = self._stream.read(expected_size)
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

    def get(self, key):
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
        return value

    def set(self, key):
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
