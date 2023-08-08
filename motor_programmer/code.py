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

leds = [Led(macropad.pixels, i) for i in range(macropad.pixels.n)]

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

    if not event.pressed:
        continue
    print(key_number)
    if key_number == 3:
        ui.advance(page_delta=-1)
    if key_number == 5:
        ui.advance(page_delta=+1)
    if key_number == 6:
        ui.advance(offset_delta=-1)
    if key_number == 8:
        ui.advance(offset_delta=+1)

    if key_number == 9:
        leds[0]["layer1"] = (255, 0, 0)

    if key_number == 10:
        leds[0]["layer2"] = (0, 255, 0)

    if key_number == 11:
        leds[0]["layer3"] = (0, 0, 255)

    if key_number == 0:
        del leds[0]["layer1"]
    if key_number == 1:
        del leds[0]["layer2"]
    if key_number == 2:
        del leds[0]["layer3"]
