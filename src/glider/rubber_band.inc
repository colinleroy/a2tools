rubber_band_WIDTH  = 7
rubber_band_HEIGHT = 4
rubber_band_BYTES  = 8
rubber_band_MIN_X  = 1
rubber_band_MAX_X  = 255-(rubber_band_WIDTH)
.assert rubber_band_MAX_X < 256, error
rubber_band_MIN_Y  = 0
rubber_band_MAX_Y  = 192-rubber_band_HEIGHT
