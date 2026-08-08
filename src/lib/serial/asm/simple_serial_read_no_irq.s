        .export         _simple_serial_read_no_irq

        .import         _serial_read_byte_direct
        .import         popptr1, returnFFFF

        .importzp       tmp1, tmp2, ptr1, ptr2

bytes      = tmp1
full_pages = tmp2
timeout    = ptr2

; int simple_serial_read_no_irq(char *buffer, size_t len)
.proc _simple_serial_read_no_irq
        sta     bytes
        stx     full_pages
        jsr     popptr1                 ; Doesn't touch X

        ldy     #$00
        sty     timeout
        sty     timeout+1

        cpx     #$00
        beq     finish_last_bytes

get_bytes:
        jsr     _serial_read_byte_direct
        bcc     store_byte_page

        inc     timeout                 ; Read no byte, inc timeout
        bne     get_bytes
        inc     timeout+1
        bne     get_bytes
        beq     timed_out

store_byte_page:                        ; Store byte,
        sta     (ptr1),y
        iny                             ; increment output,
        bne     get_bytes
        inc     ptr1+1                  ; increment output page,
        dex                             ; decrement pages
        bne     get_bytes

finish_last_bytes:
        ldx     bytes                   ; get remaining bytes to read
        beq     done
        ldy     #$00

get_last_bytes:
        jsr     _serial_read_byte_direct
        bcc     store_last_byte

        inc     timeout                 ; Read no byte, inc timeout
        bne     get_last_bytes
        inc     timeout+1
        bne     get_last_bytes
        beq     timed_out

store_last_byte:                        ; Store byte,
        sta     (ptr1),y
        iny                             ; increment output,
        dex                             ; decrement byte count
        bne     get_last_bytes

done:
        txa                             ; return 0
        rts

timed_out:
        jmp     returnFFFF

.endproc
