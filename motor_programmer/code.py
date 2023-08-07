from adafruit_macropad import MacroPad
from busio import UART
import microcontroller
from parameters import Parameters
from fake_stream import FakeStream
from ui import Ui

print("\nStartup")

# Pinout:
# https://cdn-learn.adafruit.com/assets/assets/000/104/022/original/adafruit_products_Adafruit_MacroPad_RP2040_Pinout.png?1629726427
# modbus_stream = UART(tx=microcontroller.pin.GPIO20, rx=microcontroller.pin.GPIO21, baudrate=38_400)

modbus_stream = FakeStream()
params = Parameters(modbus_stream)


macropad = MacroPad()

ui = Ui(params, macropad)
ui.render()

while True:
    event = macropad.keys.events.get()
    if not event:
        continue

    key_number = event.key_number
    macropad.pixels[key_number] = (255, 255, 255) if event.pressed else 0

    if not event.pressed:
        continue
    if key_number == 3:
        ui.advance(page_delta=-1)
    if key_number == 5:
        ui.advance(page_delta=+1)
    if key_number == 6:
        ui.advance(offset_delta=-1)
    if key_number == 8:
        ui.advance(offset_delta=+1)
