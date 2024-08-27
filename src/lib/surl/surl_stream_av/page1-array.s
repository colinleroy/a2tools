.align $40
.assert * = $7D40, error                         ; We want high byte to be odd
PAGE1_ARRAY = *                                  ; for and'ing at video_sub:@set_offset
page1_addrs_arr_high:.res (N_BASES+4+1)          ; also $7F & $55 = $55

page_addrs_arr: .byte >(page1_addrs_arr_high)     ; Inverted because we write to page 1
                .byte >(page0_addrs_arr_high)     ; when page 0 is active, and vice-versa
