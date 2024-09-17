.align $40 ; page0_ and page1_ addresses arrays must share the same low byte
PAGE0_ARRAY = *
PAGE_ARR_TO_SWITCH = >PAGE0_ARRAY - <$C054

page0_addrs_arr_high:.res (N_BASES+4+1)
pages_addrs_arr_low: .res (N_BASES+4+1)
cancelled:           .res 1
