calc_text_bases:
        ; Precalculate text lines 20-23 adresses, so we can easily jump
        ; from one to another without complicated computations. X
        ; contains line 20's base page address high byte on entry ($02 for
        ; page 0, $06 for page 1).
        ldy     #(N_BASES)    ; Y is the index - Start after HGR bases

        clc
calc_next_text_base:
        sta     pages_addrs_arr_low,y        ; Store AX
        pha
        txa
calc_addr_text_high:
        sta     page0_addrs_arr_high,y
        pla
        iny

        adc     #$80                         ; Compute next base
        bcc     :+
        inx
:       cpy     #(N_BASES+4+1)
        bcc     calc_next_text_base
        rts
