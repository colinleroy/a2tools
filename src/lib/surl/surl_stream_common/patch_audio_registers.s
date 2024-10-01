patch_audio_registers:
        lda     #<(audio_status_patches)
        sta     ptr1
        lda     #>(audio_status_patches)
        sta     ptr1+1
        lda     _ser_params + SIMPLE_SERIAL_PARAMS::DATA_SLOT
        jsr     _simple_serial_get_status_reg
        jsr     patch_addresses

        lda     #<(audio_data_patches)
        sta     ptr1
        lda     #>(audio_data_patches)
        sta     ptr1+1
        lda     _ser_params + SIMPLE_SERIAL_PARAMS::DATA_SLOT
        jsr     _simple_serial_get_data_reg
        jmp     patch_addresses
