patch_serial_registers:
        .ifdef IIGS
        brk                     ; Todo
        .else
        clc
        ldx     #$C0            ; Serial registers' high byte
        lda     #<(video_status_patches)
        sta     ptr1
        lda     #>(video_status_patches)
        sta     ptr1+1
        lda     _printer_slot
        asl
        asl
        asl
        asl
        adc     #$89            ; ACIA STATUS (in slot 0)
        jsr     patch_addresses

        lda     #<(video_data_patches)
        sta     ptr1
        lda     #>(video_data_patches)
        sta     ptr1+1
        lda     _printer_slot
        asl
        asl
        asl
        asl
        adc     #$88            ; ACIA DATA (in slot 0)
        pha
        jsr     patch_addresses
        pla
        clc
        adc     #2
        sta     vcmd+1
        stx     vcmd+2
        sta     vcmd2+1
        stx     vcmd2+2
        adc     #1
        sta     vctrl+1
        stx     vctrl+2

        lda     #<(audio_status_patches)
        sta     ptr1
        lda     #>(audio_status_patches)
        sta     ptr1+1
        lda     _data_slot
        asl
        asl
        asl
        asl
        adc     #$89
        jsr     patch_addresses

        lda     #<(audio_data_patches)
        sta     ptr1
        lda     #>(audio_data_patches)
        sta     ptr1+1
        lda     _data_slot
        asl
        asl
        asl
        asl
        adc     #$88
        pha
        jsr     patch_addresses
        pla
        clc
        adc     #2
        sta     acmd+1
        stx     acmd+2
        adc     #1
        sta     actrl+1
        stx     actrl+2
        rts
        .endif
