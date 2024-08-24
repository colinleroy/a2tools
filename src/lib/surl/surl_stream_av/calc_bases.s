calc_bases:
        ; Precalculate addresses inside pages, so we can easily jump
        ; from one to another without complicated computations. X
        ; contains the base page address's high byte on entry ($20 for
        ; page 0, $40 for page 1)
        ldy     #0              ; Y is the index - Start at base 0
        lda     #$00            ; A is the address's low byte
                                ; (and X the address's high byte)

        clc
calc_next_base:
        sta     pages_addrs_arr_low,y         ; Store AX
        pha
        txa
calc_addr_high:
        sta     page0_addrs_arr_high,y
        pla
        iny

        adc     #(MAX_OFFSET)   ; Compute next base
        bcc     :+
        inx
:       cpy     #(N_BASES)
        bcc     calc_next_base
        rts
