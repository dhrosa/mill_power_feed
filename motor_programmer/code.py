from adafruit_macropad import MacroPad
import time
from busio import UART
import microcontroller
from parameters import Parameters

print()

# Pinout:
# https://cdn-learn.adafruit.com/assets/assets/000/104/022/original/adafruit_products_Adafruit_MacroPad_RP2040_Pinout.png?1629726427
params = Parameters(
    UART(tx=microcontroller.pin.GPIO20, rx=microcontroller.pin.GPIO21, baudrate=38_400)
)

macropad = MacroPad()
text_lines = macropad.display_text(title="MacroPad Info")
pixels = macropad.pixels

print("param 4: ", params[4])

while True:
    event = macropad.keys.events.get()
    if event:
        key_number = event.key_number
        if event.pressed:
            pixels[key_number] = (255, 255, 255)
        else:
            pixels[key_number] = 0
