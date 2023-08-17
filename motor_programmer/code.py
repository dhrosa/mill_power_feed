from adafruit_macropad import MacroPad
from busio import UART
import microcontroller
from parameters import Parameters
from fake_stream import FakeStream
from ui import Ui
from led import Led
from colorsys import hls_to_rgb as hls


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


def iterable_members(cls):
    all = set()
    for name, value in cls.__dict__.items():
        if name.startswith("_"):
            continue
        if isinstance(value, type):
            all |= value.all
        else:
            all.add(value)
    cls.all = all
    return cls


@iterable_members
class buttons:
    @iterable_members
    class page:
        previous = 3
        next = 5

    @iterable_members
    class parameter:
        previous = 6
        next = 8

    cancel = 9
    confirm = 11


pressed_colors = [0] * len(leds)

for i in buttons.page.all:
    leds[i]["base"] = hls(5 / 6, 0.25, 0.75)
    pressed_colors[i] = hls(5 / 6, 0.5, 1)

for i in buttons.parameter.all:
    leds[i]["base"] = hls(4 / 6, 0.25, 0.75)
    pressed_colors[i] = hls(4 / 6, 0.5, 1)

leds[buttons.cancel]["base"] = hls(0, 0.25, 0.75)
pressed_colors[buttons.cancel] = hls(0, 0.5, 1)

leds[buttons.confirm]["base"] = hls(1 / 3, 0.5, 0.75)
pressed_colors[buttons.confirm] = hls(1 / 3, 0.5, 1)

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

    button = event.key_number
    led = leds[button]

    if event.released:
        del led["press"]
        continue

    led["press"] = pressed_colors[button]

    if button == buttons.page.previous:
        ui.advance(page_delta=-1)
    if button == buttons.page.next:
        ui.advance(page_delta=+1)
    if button == buttons.parameter.previous:
        ui.advance(offset_delta=-1)
    if button == buttons.parameter.next:
        ui.advance(offset_delta=+1)
    if button == buttons.cancel:
        ui.cancel()
    if button == buttons.confirm:
        ui.confirm()
