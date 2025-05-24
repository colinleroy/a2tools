patch_audio_registers:
        lda     #$08
        bit     ostype
        bpl     :+
        lda     #$01
:       sta     has_byte

        lda     #<(audio_status_patches)
        sta     ptr1
        lda     #>(audio_status_patches)
        sta     ptr1+1
        lda     _ser_status_reg
        ldx     _ser_status_reg+1
        jsr     patch_addresses

        lda     #<(audio_data_patches)
        sta     ptr1
        lda     #>(audio_data_patches)
        sta     ptr1+1
        lda     _ser_data_reg
        ldx     _ser_data_reg+1
        jmp     patch_addresses
