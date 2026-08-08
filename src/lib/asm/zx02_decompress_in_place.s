        .export     _zx02_decompress_in_place

        .import   _open, _read, _close, _memmove
        .import   pushax, pusha0, popax
        .import   _decompress_zx02

        .include    "apple2.inc"
        .include  "fcntl.inc"

        .segment "CODE"

.proc _zx02_decompress_in_place
        sta     end_addr
        stx     end_addr+1

        jsr     popax
        sta     start_addr
        stx     start_addr+1

        ; Keep filename in TOS, and push IO mode
        lda     #<O_RDONLY
        jsr     pusha0

        ldy     #$04          ; _open is variadic, tell it how many bytes to pop
        jsr     _open
        cmp     #$FF          ; did it work?
        bne     :+
        tax                   ; Open error
        rts

:       jsr     pushax        ; We now have an fd. Push it for _read
        jsr     pushax        ; and a second time for _close

        lda     start_addr    ; Where to read to?
        ldx     start_addr+1
        jsr     pushax

        sec                   ; compute max length
        lda     end_addr
        sbc     start_addr
        tay
        lda     end_addr+1
        sbc     start_addr+1
        tax                   ; and push it to _read
        tya

        jsr     _read
        sta     comp_len
        pha                   ; Push to check after close
        txa
        sta     comp_len+1
        pha

        jsr     popax         ; Get fd back from stack
        jsr     _close        ; And close the file

        pla
        cmp     #$FF
        bne     :+
        pla                   ; Uh-oh, _read return $FFxx... pop A
        tax
        rts                   ; and return -1

:       pla                   ; No read error, pop A and continue

        sec                   ; compute where to move compressed data
        lda     end_addr
        sbc     comp_len
        tay
        lda     end_addr+1
        sbc     comp_len+1
        tax
        tya
        jsr     pushax        ; and push it
        jsr     pushax        ; Push it again for decompression

        lda     start_addr
        ldx     start_addr+1
        jsr     pushax

        lda     comp_len      ; length to move
        ldx     comp_len+1
        jsr     _memmove      ; Do the move

        lda     start_addr
        ldx     start_addr+1
        jsr     _decompress_zx02
        lda     #$00
        tax
        rts
.endproc

        .segment "BSS"
start_addr:     .res 2
end_addr:       .res 2
comp_len:       .res 2
