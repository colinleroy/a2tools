; -----------------------------------------------------------------
; Sneak function in available hole
setup:
        ; Setup pointer access to SPKR
        lda     #<(SPKR)
        sta     spkr_ptr
        lda     #>(SPKR)
        sta     spkr_ptr+1

        ; Calculate bases for HGR page 0
        lda     #>(page0_addrs_arr_high)
        sta     calc_addr_high+2
        sta     calc_addr_text_high+2
        ldx     #PAGE0_HB
        jsr     calc_bases
        lda     #$50
        ldx     #$06
        jsr     calc_text_bases

        ; Calculate bases for HGR page 1
        lda     #>(page1_addrs_arr_high)
        sta     calc_addr_high+2
        sta     calc_addr_text_high+2
        ldx     #PAGE1_HB
        jsr     calc_bases
        lda     #$50
        ldx     #$0A
        jsr     calc_text_bases

        ; Setup serial registers
        jsr     patch_audio_registers
        jsr     patch_video_registers

        lda     #$2F            ; Surl client ready
        jsr     _serial_putc_direct

        lda     #SAMPLE_OFFSET  ; Inform server about sample offset
        jsr     _serial_putc_direct

        lda     #SAMPLE_MULT    ; Inform server about sample multiplier
        jsr     _serial_putc_direct

        jsr     _serial_read_byte_no_irq
        sta     cur_mix

        tay
        lda     $C052,y

        lda     #<(page0_addrs_arr_high)
        sta     page_ptr_high
        lda     #>(page0_addrs_arr_high)
        sta     page_ptr_high+1

        ; Init vars
        lda     #$40
        sta     cur_base+1
        lda     #$00
        sta     cur_base
        sta     kbd_cmd
        sta     cur_mix
        sta     next_offset
        sta     cancelled

        .ifndef __APPLE2ENH__
        sta     next            ; Clear low byte of next pointer
        .endif

        rts
