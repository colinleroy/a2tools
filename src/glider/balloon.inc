balloon_WIDTH  = 21
balloon_HEIGHT = 21
balloon_BYTES  = 84
balloon_MIN_X  = 1
balloon_MAX_X  = 255-(balloon_WIDTH)
.assert balloon_MAX_X < 256, error
balloon_MIN_Y  = 0
balloon_MAX_Y  = 192-balloon_HEIGHT
