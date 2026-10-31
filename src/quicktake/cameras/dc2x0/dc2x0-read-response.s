        .export             _dc2x0_read_response

        .import             _simple_serial_read_no_irq
        .import             _serial_read_byte_no_irq

        .include            "stdio.inc"

.segment "DC2X0"

.proc _dc2x0_read_response
        jsr       _serial_read_byte_no_irq
        cmp       #$01
        bne       err
        jsr       _simple_serial_read_no_irq
        cmp       #<EOF
        bne       :+
err:    tax
        rts
:       jsr       _serial_read_byte_no_irq

        lda       #$00
        tax
        rts
.endproc
