        .export             _dc50_read_response

        .import             _simple_serial_read_no_irq
        .import             _serial_read_byte_no_irq

        .include            "stdio.inc"

.segment "DC50"

.proc _dc50_read_response
        jsr       _simple_serial_read_no_irq
        cmp       #<EOF
        bne       :+
        tax
        rts
:       jsr       _serial_read_byte_no_irq

        lda       #$00
        tax
        rts
.endproc
