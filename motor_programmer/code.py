from adafruit_macropad import MacroPad
import time
from parameters import Parameters

print()

macropad = MacroPad()
text_lines = macropad.display_text(title="MacroPad Info")
pixels = macropad.pixels

params = Parameters()
print("param 4: ", params[4])

while True:
    event = macropad.keys.events.get()
    if event:
        key_number = event.key_number
        if event.pressed:
            pixels[key_number] = (255, 255, 255)
        else:
            pixels[key_number] = 0
