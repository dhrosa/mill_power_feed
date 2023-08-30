from modbus import Modbus
import struct
from collections import namedtuple

REGISTER_READ = 0
REGISTER_WRITE = 1

Request = namedtuple('Request', ['command', 'value', 'offset'])

class Target:
    def __init__(self, i2c_target, stream):
        self._i2c_target = i2c_target
        self._modbus = Modbus(stream)
        self._buffered_input = bytearray()


    def listen(self):
        while True:
            request = self._i2c_target.request()
            if not request:
                continue
            with request:
                self._handle_request(request)


    def _handle_request(self, request):
        if not request.is_read:
            # Controller -> Device
            message = request.read()
            print(f"Write request contents: {list(message)}")
            self._buffered_input.extend(message)
            return
        # Device -> Controller
        head = self._buffered_input
        self._buffered_input = bytearray()
        if len(head) != 5:
            print(f'Ignoring malformed message: {list(head)}')
            return
        print(f'Servicing read for request: {list(head)}')
        command, offset, value = struct.unpack('<BHH', head)
        if command == REGISTER_READ:
            value = self._modbus.get(offset)
        elif command == REGISTER_WRITE:
            self._modbus.set(offset, value)
        success = True
        response = struct.pack('<BH', success, value)
        print(f"Responding to read request with: {list(response)}")
        request.write(response)
