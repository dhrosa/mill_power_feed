from collections import namedtuple

Overlay = namedtuple("Overlay", ["key", "value"])


class Led:
    def __init__(self, pixels, index):
        self._pixels = pixels
        self._index = index

        self._overlays = []

    def _update(self):
        value = 0
        if self._overlays:
            value = self._overlays[-1].value
        self._pixels[self._index] = value

    def __getitem__(self, key):
        for overlay in self._overlays:
            if overlay.key == key:
                return overlay.value
        return None

    def __setitem__(self, key, value):
        for i, overlay in enumerate(self._overlays):
            if overlay.key == key:
                self._overlays[i] = Overlay(key, value)
                self._update()
                return
        self._overlays.append(Overlay(key, value))
        self._update()

    def __delitem__(self, key):
        for i, overlay in enumerate(self._overlays):
            if overlay.key == key:
                del self._overlays[i]
                self._update()
                return
