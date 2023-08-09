from adafruit_macropad import MacroPad
from busio import UART
import microcontroller
from parameters import Parameters
from fake_stream import FakeStream
from ui import Ui
from led import Led

print("\nStartup")

# Pinout:
# https://cdn-learn.adafruit.com/assets/assets/000/104/022/original/adafruit_products_Adafruit_MacroPad_RP2040_Pinout.png?1629726427
# modbus_stream = UART(tx=microcontroller.pin.GPIO20, rx=microcontroller.pin.GPIO21, baudrate=38_400)

modbus_stream = FakeStream()
params = Parameters(modbus_stream)


macropad = MacroPad()

ui = Ui(params, macropad)
ui.render()

macropad.pixels.brightness = 0.5
leds = [Led(macropad.pixels, i) for i in range(macropad.pixels.n)]

for i in (3, 5):
    leds[i]['base'] = (0, 32, 0)

for i in (6, 8):
    leds[i]['base'] = (0, 16, 16)

last_encoder_value = macropad.encoder
while True:
    macropad.encoder_switch_debounced.update()
    encoder_value = macropad.encoder
    if encoder_value != last_encoder_value:
        ui.increment_digit(encoder_value - last_encoder_value)
        last_encoder_value = encoder_value

    if macropad.encoder_switch_debounced.pressed:
        ui.next_digit()
    event = macropad.keys.events.get()
    if not event:
        continue

    key_number = event.key_number
    # macropad.pixels[key_number] = (255, 255, 255) if event.pressed else 0

    if key_number == 3:
        if event.pressed:
            ui.advance(page_delta=-1)
            leds[3]['press'] = (0, 255, 0)
        else:
            del leds[3]['press']

    if key_number == 5:
        if event.pressed:
            ui.advance(page_delta=+1)
            leds[5]['press'] = (0, 255, 0)
        else:
            del leds[5]['press']

    if key_number == 6:
        if event.pressed:
            ui.advance(offset_delta=-1)
            leds[6]['press'] = (0, 128, 128)
        else:
            del leds[6]['press']
    if key_number == 8:
        if event.pressed:
            ui.advance(offset_delta=+1)
            leds[8]['press'] = (0, 128, 128)
        else:
            del leds[8]['press']
