switch_WIDTH  = 7
switch_HEIGHT = 10
switch_BYTES  = 20
switch_MIN_X  = 1
switch_MAX_X  = 255-(switch_WIDTH)
.assert switch_MAX_X < 256, error
switch_MIN_Y  = 0
switch_MAX_Y  = 192-switch_HEIGHT
