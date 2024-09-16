patch_video_registers:
        .ifdef IIGS
        brk                     ; Todo
        .else
        lda     #<(video_status_patches)
        sta     ptr1
        lda     #>(video_status_patches)
        sta     ptr1+1
        lda     _printer_slot
        jsr     _simple_serial_get_status_reg
        jsr     patch_addresses

        lda     #<(video_data_patches)
        sta     ptr1
        lda     #>(video_data_patches)
        sta     ptr1+1
        lda     _printer_slot
        jsr     _simple_serial_get_data_reg
        jsr     patch_addresses

        lda     _printer_slot
        jsr     _simple_serial_get_cmd_reg
        sta     vcmd+1
        sta     vcmd2+1
        stx     vcmd+2
        stx     vcmd2+2

        lda     _printer_slot
        jsr     _simple_serial_get_ctrl_reg
        sta     vctrl+1
        stx     vctrl+2

        lda     _data_slot
        jsr     _simple_serial_get_cmd_reg
        sta     acmd+1
        stx     acmd+2

        lda     _data_slot
        jsr     _simple_serial_get_ctrl_reg
        sta     actrl+1
        stx     actrl+2

acmd:   lda     $A8FF           ; Copy command and control registers from
vcmd:   sta     $98FF           ; the main serial port to the second serial
actrl:  lda     $A8FF           ; port, it's easier than setting it up from
vctrl:  sta     $98FF           ; scratch

        rts
        .endif
