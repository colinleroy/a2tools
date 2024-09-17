.align $40
PAGE1_ARRAY = *
.assert PAGE1_ARRAY = PAGE0_ARRAY + $100, error
.assert PAGE_ARR_TO_SWITCH = >PAGE1_ARRAY - <$C055, error

page1_addrs_arr_high:.res (N_BASES+4+1)

page_addrs_arr: .byte >(page1_addrs_arr_high)     ; Inverted because we write to page 1
                .byte >(page0_addrs_arr_high)     ; when page 0 is active, and vice-versa
