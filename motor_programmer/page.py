# Maps to a 'page' of parameters in the reference UI
class Page:
    def __init__(self, params, number, start, stop):
        self.params = params
        self.number = number
        self.start = start
        self.stop = stop

    def __str__(self):
        return f"<Page {self.number}: [{self.start}, {self.stop})>"

    __repr__ = __str__

    def __len__(self):
        return self.stop - self.start

    def __getitem__(self, offset):
        return self.params[self.absolute_key(offset)]

    def __setitem__(self, offset, value):
        self.params[self.start + self.absolute_key(offset)] = value

    def absolute_key(self, key):
        return self.start + (key % len(self))
