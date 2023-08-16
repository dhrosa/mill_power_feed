from displayio import Group
from adafruit_display_text import label, bitmap_label
import terminalio
import os
from adafruit_bitmap_font import bitmap_font
from font_loader import load_fonts

fonts = load_fonts()
font = fonts[12]

def make_label(*args, **kwargs):
    kwargs.update({"font": font})
    return label.Label(*args, **kwargs)


def translate(pos, delta):
    return (pos[0] + delta[0], pos[1] + delta[1])


class Digit(label.Label):
    def __init__(self):
        super().__init__(font=font)
        self.value = 0
        self.selected = False

    @property
    def value(self):
        return self._value

    @value.setter
    def value(self, new_value):
        self._value = new_value
        self.text = str(new_value)

    @property
    def selected(self):
        return self._selected

    @selected.setter
    def selected(self, new_selected):
        self._selected = new_selected
        black = 0
        white = 0xFF_FF_FF
        if new_selected:
            self.color = black
            self.background_color = white
        else:
            self.color = white
            self.background_color = black


class ValueEditor(Group):
    def __init__(self):
        super().__init__()
        self._selected_index = 5
        x = 0
        y = 0
        for i in reversed(range(6)):
            digit = Digit()
            digit.value = i
            digit.anchor_point = (0, 0)
            digit.anchored_position = (x, y)
            x += digit.width
            self.append(digit)
        self.value = 123

    @property
    def value(self):
        return int("".join(d.text for d in self))

    @value.setter
    def value(self, new_value):
        value_str = f"{new_value:6d}"
        for i, digit_str in enumerate(value_str):
            digit = self[i]
            if digit_str == " ":
                digit.value = 0
                digit.hidden = True
            else:
                digit.value = int(digit_str)
                digit.hidden = False
        self[5].selected = True

    @property
    def selected_digit(self):
        return self[self._selected_index]

    def increment_digit(self, delta):
        if delta == 0:
            return
        digit = self.selected_digit
        digit.value = (digit.value + delta) % 10

    def next_digit(self):
        self.selected_digit.selected = False
        self._selected_index -= 1
        self._selected_index %= 6
        self.selected_digit.selected = True
        self.selected_digit.hidden = False


class Ui:
    def __init__(self, params, macropad):
        self._params = params
        self._macropad = macropad
        self._page_index = 0
        self._offset = 0

        width = macropad.display.width
        height = macropad.display.height

        self._root = Group()

        title = make_label(text="Parameter Editor")
        self._root.append(title)
        title.anchor_point = (0.5, 0)
        title.anchored_position = (width // 2, 0)

        form = Group(y=title.anchored_position[1] + title.height)
        self._root.append(form)

        key_label = make_label(text="Key: ")
        form.append(key_label)
        key_label.anchor_point = (1, 0)
        key_label.anchored_position = (width // 2, 0)

        value_label = make_label(text="Value: ")
        form.append(value_label)
        value_label.anchor_point = key_label.anchor_point
        value_label.anchored_position = translate(
            key_label.anchored_position, (0, key_label.height)
        )

        new_value_label = make_label(text="New Value: ")
        form.append(new_value_label)
        new_value_label.anchor_point = value_label.anchor_point
        new_value_label.anchored_position = translate(
            value_label.anchored_position, (0, value_label.height)
        )

        key = make_label(text="<KEY>")
        form.append(key)
        key.anchor_point = (0, 0)
        key.anchored_position = (width // 2, 0)
        self._key_text = key

        value = make_label(text="<VALUE>")
        form.append(value)
        value.anchor_point = (0, 0)
        value.anchored_position = translate(key.anchored_position, (0, key.height))
        self._value_text = value

        new_value = ValueEditor()
        form.append(new_value)
        new_value.x = value.anchored_position[0]
        new_value.y = value.anchored_position[1] + value.height
        self._new_value_editor = new_value

        self._update_key_value_text()

    @property
    def page(self):
        return self._params.pages[self._page_index]

    def _update_key_value_text(self):
        page = self.page
        key = self.page.absolute_key(self._offset)
        offset = self._offset
        value = page[offset]

        self._key_text.text = f"{page.number}.{offset:02d} ({key:3d})"
        self._value_text.text = f"{value:6d}"
        self._new_value_editor.value = value

    def render(self):
        self._macropad.display.root_group = self._root

    def advance(self, page_delta=0, offset_delta=0):
        if offset_delta:
            assert not page_delta
            self._offset += offset_delta
            self._offset %= len(self.page)
        if page_delta:
            assert not offset_delta
            self._offset = 0
            self._page_index += page_delta
            self._page_index %= len(self._params.pages)
        self._update_key_value_text()
        self.render()

    def increment_digit(self, delta):
        self._new_value_editor.increment_digit(delta)

    def next_digit(self):
        self._new_value_editor.next_digit()
