import struct
import crc


# Replacement for real modbus UART for quick protoyping
class FakeStream:
    def __init__(self):
        self._last_request = None
        self._data = {}

    def write(self, request):
        self._last_request = request

    def read(self, nbytes):
        last_request = self._last_request
        self._last_request = None
        if not last_request:
            return None
        parts = struct.unpack(">BBHHBB", last_request)
        function = parts[1]
        if function == 3:
            key = parts[2]
            value = self._data.get(key, 1000 + key)
            payload = struct.pack(">BBBH", 63, 3, 2, value)
            return payload + crc.checksum_bytes(payload)
        if function == 6:
            key = parts[2]
            value = parts[3]
            self._data[key] = value
            return last_request
