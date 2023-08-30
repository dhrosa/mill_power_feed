import msgpack
import struct
from array import array
from adafruit_bus_device.i2c_device import I2CDevice

REGISTER_READ = 0
REGISTER_WRITE = 1

class I2cBridge:
    def __init__(self, i2c):
        self._device = I2CDevice(i2c, 0x69)

    def _transfer(self, command, key, value):
        request = struct.pack("<BHH", command, key, value)
        response = bytearray([0] * 3)
        with self._device:
            self._device.write_then_readinto(request, response)
        success, value = struct.unpack("<BH", response)
        if success:
            return value
        else:
            return None

    def __getitem__(self, key):
        return self._transfer(REGISTER_READ, key, value=0)

    def __setitem__(self, key, value):
        written_value = self._transfer(REGISTER_WRITE, key, value)
        if written_value is None:
            raise RuntimeError("unknown write failure")
