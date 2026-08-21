        .export             _dc50_read_response

        .import             _buffer
        .import             _simple_serial_read_no_irq
        .import             _serial_read_byte_no_irq

        .import             pushax
        .importzp           ptr1

        .include            "stdio.inc"
        .include            "dc50.inc"

.proc _dc50_read_response
        bne       :+
        cpx       #$00
        bne       :+
        rts

:       sta       ptr1
        stx       ptr1+1

        lda       #<_buffer
        ldx       #>_buffer
        jsr       pushax
        lda       ptr1
        ldx       ptr1+1

        jsr       _simple_serial_read_no_irq
        cmp       #<EOF
        bne       :+

        tax
        rts

:       jsr       _serial_read_byte_no_irq ; read checksum
        lda       #$00
        tax
        rts
.endproc
