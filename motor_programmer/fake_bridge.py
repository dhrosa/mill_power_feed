import struct

class FakeBridge:
    def __init__(self):
        self._data = {}

    def __getitem__(self, key):
        return self._data.get(key, 1000 + key)

    def __setitem__(self, key, value):
        self._data[key] = value
