from adafruit_macropad import MacroPad
import microcontroller
from parameters import Parameters
from ui import Ui
from led import Led
from colorsys import hls_to_rgb as hls
from fake_bridge import FakeBridge


print("\nStartup")

modbus_bridge = FakeBridge()
params = Parameters(modbus_bridge)

macropad = MacroPad()

ui = Ui(params, macropad)
ui.render()

macropad.pixels.brightness = 1
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

    @iterable_members
    class confirm:
        a = 9
        b = 10
        c = 11

    cancel = 0


pressed_colors = [0] * len(leds)

for i in buttons.page.all:
    leds[i]["base"] = hls(1/ 2, 0.25, 0.75)
    pressed_colors[i] = hls(1 / 2, 0.5, 1)

for i in buttons.parameter.all:
    leds[i]["base"] = hls(4 / 6, 0.25, 0.75)
    pressed_colors[i] = hls(4 / 6, 0.5, 1)

leds[buttons.cancel]["base"] = hls(0, 0.25, 0.75)
pressed_colors[buttons.cancel] = hls(0, 0.5, 1)

for i in buttons.confirm.all:
    leds[i]["base"] = hls(1 / 3, 0.25, 0.75)
    pressed_colors[i] = hls(1 / 3, 0.5, 1)

last_encoder_value = macropad.encoder
confirm_pressed = set()

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
        confirm_pressed.discard(button)
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
    if button in buttons.confirm.all:
        confirm_pressed.add(button)
        if confirm_pressed == buttons.confirm.all:
            confirm_pressed.clear()
            ui.confirm()
