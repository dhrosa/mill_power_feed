import displayio
from adafruit_display_text import label, bitmap_label
import terminalio


def make_label(*args, **kwargs):
    kwargs.update({"font": terminalio.FONT})
    return label.Label(*args, **kwargs)


def translate(pos, delta):
    return (pos[0] + delta[0], pos[1] + delta[1])


class Ui:
    def __init__(self, params, macropad):
        self._params = params
        self._macropad = macropad
        self._page_index = 0
        self._offset = 0

        width = macropad.display.width
        height = macropad.display.height

        self._root = displayio.Group()

        title = make_label(text="Parameter Editor")
        self._root.append(title)
        title.anchor_point = (0.5, 0)
        title.anchored_position = (width / 2, 0)

        form = displayio.Group(y=title.anchored_position[1] + title.height)
        self._root.append(form)

        key_label = make_label(text="Key: ")
        form.append(key_label)
        key_label.anchor_point = (1, 0)
        key_label.anchored_position = (width / 2, 0)

        value_label = make_label(text="Value: ")
        form.append(value_label)
        value_label.anchor_point = key_label.anchor_point
        value_label.anchored_position = translate(
            key_label.anchored_position, (0, key_label.height)
        )

        key = make_label(text="<KEY>")
        form.append(key)
        key.anchor_point = (0, 0)
        key.anchored_position = (width / 2, 0)
        self._key_text = key

        value = make_label(text="<VALUE>")
        form.append(value)
        value.anchor_point = (0, 0)
        value.anchored_position = translate(key.anchored_position, (0, key.height))
        self._value_text = value

        self._update_key_value_text()

    @property
    def page(self):
        return self._params.pages[self._page_index]

    def _update_key_value_text(self):
        page = self.page
        key = self.page.absolute_key(self._offset)
        offset = self._offset

        self._key_text.text = f"{page.number}.{offset:02d} ({key:3d})"
        self._value_text.text = f"{page[offset]:6d}"

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