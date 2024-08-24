.align $40 ; page0_ and page1_ addresses arrays must share the same low byte
.assert * = $7C40, error                         ; We want this one's high byte to be even
PAGE0_ARRAY = *                                  ; for and'ing at video_sub:@set_offset
page0_addrs_arr_high:.res (N_BASES+4+1)          ; Also $7C & $55 = $54
pages_addrs_arr_low: .res (N_BASES+4+1)
