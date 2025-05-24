patch_video_registers:
        ; Open aux port
        lda     _ser_params + SIMPLE_SERIAL_PARAMS::PRINTER_SLOT
        ldx     #SER_BAUD_115200
        jsr     _serial_open

        lda     #<(video_status_patches)
        sta     ptr1
        lda     #>(video_status_patches)
        sta     ptr1+1
        lda     _ser_status_reg
        ldx     _ser_status_reg+1
        jsr     patch_addresses

        lda     #<(video_data_patches)
        sta     ptr1
        lda     #>(video_data_patches)
        sta     ptr1+1
        lda     _ser_data_reg
        ldx     _ser_data_reg+1
        jsr     patch_addresses


        ; Reopen data port
        lda     _ser_params + SIMPLE_SERIAL_PARAMS::DATA_SLOT
        ldx     #SER_BAUD_115200
        jsr     _serial_open
        ; And re-enable IRQs
        lda       #1
        jsr       _simple_serial_set_irq

        rts
