spkr_ptr      = _zp6            ; word - Pointer to SPKR to access in 5 cycles
next_offset   = _zp8            ; byte - Where to write next video byte
cur_mix       = _zp9            ; byte - HGR MIX status (for subtitles toggling)

cur_base      = ptr1            ; word - Current HGR base to write to
page_ptr_high = ptr4            ; word - Pointer to bases addresses (high byte) array
wasted        = tmp1            ; byte - A zero in zero page (mostly to waste 3 cycles)
kbd_cmd       = tmp2            ; byte - Deferred keyboard command to handle
