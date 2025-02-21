clock_WIDTH  = 14
clock_HEIGHT = 14
clock_BYTES  = 42
clock_MIN_X  = 1
clock_MAX_X  = 255-(clock_WIDTH)
.assert clock_MAX_X < 256, error
clock_MIN_Y  = 0
clock_MAX_Y  = 192-clock_HEIGHT
