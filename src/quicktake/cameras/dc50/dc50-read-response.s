        .export             _dc50_read_response
        .import             _response_len, _response_continues

        .import             _buffer
        .import             _simple_serial_read_no_irq
        .import             _serial_read_byte_no_irq

        .import             pushax
        .importzp           ptr1

        .include            "stdio.inc"
        .include            "dc50.inc"

.proc _dc50_read_response
        lda       #<_buffer   ; read header
        ldx       #>_buffer
        jsr       pushax
        lda       #6
        ldx       #0

        jsr       _simple_serial_read_no_irq
        cmp       #<EOF
        bne       :+
err_out:
        lda       #<EOF
        tax
        rts

:       lda       _buffer+0   ; does the response look good?
        cmp       #_ESC
        bne       err_out
        lda       _buffer+1
        cmp       #_STX
        bne       err_out

        lda       #>_buffer   ; Reset address for read
        sta       store_byte+2
        ldy       #$00        ; Set index

        ldx       _buffer+5   ; Remember length of response
        stx       _response_len+1
        stx       ptr1        ; number of pages
        lda       _buffer+4
        sta       _response_len
        tax                   ; number of bytes to read in the first page

next_byte:
        bne       :+          ; More bytes in the page?
        dec       ptr1        ; Check for more pages
        bmi       done

:       jsr       _serial_read_byte_no_irq
        cmp       #_ESC       ; Skip first ESC
        bne       store_byte
        jsr       _serial_read_byte_no_irq
store_byte:
        sta       _buffer,y
        iny
        bne       :+
        inc       store_byte+2; Prepare for next page

:       dex
        jmp       next_byte

done:
        jsr       _serial_read_byte_no_irq  ; Discard first byte of footer
        jsr       _serial_read_byte_no_irq  ; check if second == ETB
        ldx       #0
        cmp       #_ETB
        bne       :+
        inx
:       stx       _response_continues
        jsr       _serial_read_byte_no_irq  ; Discard last byte of footer
        lda       #$00
        tax
        rts
.endproc
