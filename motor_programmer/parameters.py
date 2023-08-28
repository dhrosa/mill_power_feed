from page import Page

class Parameters:
    def __init__(self, bridge_device):
        self._device = bridge_device
        self._cache = {}
        starts = (0, 25, 65, 95, 125, 175, 215, 255)
        lengths = (20, 38, 24, 25, 48, 36, 34, 17)
        self._pages = []
        for i, (start, length) in enumerate(zip(starts, lengths)):
            self._pages.append(Page(self, i, start, start + length))

    @property
    def pages(self):
        return self._pages

    def __len__(self):
        return 304

    def __getitem__(self, key):
        key = self._check_key(key)
        if key in self._cache:
            return self._cache[key]
        value = self._device[key]
        self._cache[key] = value
        return value

    def __setitem__(self, key, value):
        key = self._check_key(key)
        self._device[key] = value
        self._cache[key] = value


    def _check_key(self, key):
        if int(key) != key:
            raise TypeError(f"Non-integer key: {key}")
        return int(key) % len(self)
