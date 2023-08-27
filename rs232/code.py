import board
from i2ctarget import I2CTarget
from fake_stream import FakeStream
from target import Target

ADDRESS = 0x69  # nice

stream = FakeStream()

with I2CTarget(board.SCL, board.SDA, (ADDRESS,)) as i2c_target:
    target = Target(i2c_target, stream)
    target.listen()
