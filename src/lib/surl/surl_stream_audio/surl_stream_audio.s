_surl_stream_audio:
        php
        sei                     ; Disable all interrupts

        ; Setup pointers
        jsr     setup_pointers
        ; Patch serial registers
        jsr     patch_audio_registers
        jsr     patch_vu_meters

        lda     #$2F                    ; Ready
        jsr     _serial_putc_direct

        lda     numcols                 ; Set numcols on proxy
        jsr     _serial_putc_direct
        lda     #SAMPLE_OFFSET          ; Send sample base
        jsr     _serial_putc_direct
        lda     #SAMPLE_MULT            ; Send sample multiplier
        jsr     _serial_putc_direct

        ; Start with silence
        jmp     silence
