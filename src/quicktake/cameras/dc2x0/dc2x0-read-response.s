        .export             _dc2x0_read_response

        .import             _simple_serial_read_no_irq
        .import             _serial_read_byte_no_irq

        .import             returnFFFF, popax
        .include            "stdio.inc"

.segment "DC2X0"

.proc _dc2x0_read_response
        sta       len_l
        stx       len_h
        jsr       _serial_read_byte_no_irq
        cmp       #$01
        bne       err_pop
len_l = *+1
        lda       #$00
len_h = *+1
        ldx       #$00
        jsr       _simple_serial_read_no_irq
        cmp       #<EOF
        bne       :+
        tax
        rts
:       jsr       _serial_read_byte_no_irq

        lda       #$00
        tax
        rts
err_pop:
        jsr       popax
        jmp       returnFFFF
.endproc
