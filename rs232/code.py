import board
from i2ctarget import I2CTarget
from fake_stream import FakeStream
from target import Target

ADDRESS = 0x69

stream = FakeStream()

print("\nRS232 bridge startup")

with I2CTarget(board.SCL1, board.SDA1, (ADDRESS,)) as i2c_target:
    print(f"Listening on i2c address {ADDRESS:#x}")
    target = Target(i2c_target, stream)
    target.listen()
