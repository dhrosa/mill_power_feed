from modbus import Modbus


class Target:
    def __init__(self, i2c_target, stream):
        self._i2c_target = i2c_target
        self._modbus = Modbus(stream)

    def _handle_request(self, request):
        pass

    def listen(self):
        while True:
            request = self._i2c_target.request()
            if not request:
                continue
            with request:
                self._handle_request(request)
