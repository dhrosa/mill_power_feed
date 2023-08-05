from adafruit_macropad import MacroPad
import time
from busio import UART
import microcontroller
from parameters import Parameters

print()


class Ui:
    def __init__(self, params, macropad):
        self._params = params
        self._macropad = macropad
        self._text = macropad.display_text(title="Parameter Editor")
        self._page_index = 0
        self._offset = 0

    @property
    def page(self):
        return self._params.pages[self._page_index]

    def render(self):
        page = self.page
        key = self.page.absolute_key(self._offset)
        self._text[0].text = f"  Key: {page.number}.{self._offset:02d} ({key:3d})"
        self._text[1].text = f"Value: {page[self._offset]:6d}"
        self._text.show()

    def advance_page(self, delta):
        if delta == 0:
            return
        self._offset = 0
        self._page_index += delta
        self._page_index %= len(self._params.pages)
        self.render()

    def advance_offset(self, delta):
        if delta == 0:
            return
        self._offset += delta
        self._offset %= len(self.page)
        self.render()


# Pinout:
# https://cdn-learn.adafruit.com/assets/assets/000/104/022/original/adafruit_products_Adafruit_MacroPad_RP2040_Pinout.png?1629726427
params = Parameters(
    UART(tx=microcontroller.pin.GPIO20, rx=microcontroller.pin.GPIO21, baudrate=38_400)
)

macropad = MacroPad()
text_lines = macropad.display_text(title="Parameter Editor")

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
        ui.advance_page(-1)
    if key_number == 5:
        ui.advance_page(+1)
    if key_number == 6:
        ui.advance_offset(-1)
    if key_number == 8:
        ui.advance_offset(+1)
