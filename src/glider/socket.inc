socket_WIDTH  = 7
socket_HEIGHT = 11
socket_BYTES  = 22
socket_MIN_X  = 1
socket_MAX_X  = 255-(socket_WIDTH)
.assert socket_MAX_X < 256, error
socket_MIN_Y  = 0
socket_MAX_Y  = 192-socket_HEIGHT
