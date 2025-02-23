rubber_box_WIDTH  = 14
rubber_box_HEIGHT = 13
rubber_box_BYTES  = 39
rubber_box_MIN_X  = 1
rubber_box_MAX_X  = 255-(rubber_box_WIDTH)
.assert rubber_box_MAX_X < 256, error
rubber_box_MIN_Y  = 0
rubber_box_MAX_Y  = 192-rubber_box_HEIGHT
