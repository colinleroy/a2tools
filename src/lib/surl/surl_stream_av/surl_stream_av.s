_surl_stream_av:                ; Entry point
        php
        sei                     ; Disable all interrupts

        pha

        lda     #$00            ; Disable serial interrupts
        jsr     _simple_serial_set_irq

        pla
        ; Setup pointers
        jsr     setup

        ; Clear pages
        bit     $C082
        lda     #$40
        sta     $E6
        jsr     $F3F2
        lda     #$20
        sta     $E6
        jsr     $F3F2
        bit     $C080

        lda     #$2F            ; Surl client ready
        jsr     _serial_putc_direct

        clv                     ; clear offset-received flag

ass:    lda     $A9FF           ; Wait for an audio byte
        and     #HAS_BYTE
        beq     ass
ads:    ldx     $A8FF
        JUMP_NEXT_DUTY  
