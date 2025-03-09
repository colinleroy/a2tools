        .export    _print_char
        .export    _print_string
        .export    _read_string
        .export    _str_input

        .import    _font, _font_mask
        .import    _hgr_hi, _hgr_low

        .import    popptr1
        .importzp  ptr1, tmp1, tmp2, tmp3

        .include   "apple2.inc"
        .include   "font.gen.inc"

.segment "LOWCODE"

; A: ASCII code of the char
; X: X coord / 7
; Y: bottom Y coord
.proc _print_char
        sec
        sbc     #' '              ; Space is our first character
        stx     char_x_offset+1
        asl
        tax
        lda     _font,x
        sta     char_data+1
        lda     _font+1,x
        sta     char_data+2

        ldx     #4
        clc

:       lda     _hgr_low,y
char_x_offset:
        adc     #$FF
        sta     scr+1
        lda     _hgr_hi,y
        adc     #0
        sta     scr+2

char_data:
        lda     $FFFF,x
scr:
        sta     $FFFF
        dey
        dex
        bpl     :-
        rts
.endproc

; TOS: Address of the buffer to print
; X: X coord / 7
; Y: bottom Y coord
; Returns with updated X
.proc _print_string
        stx     tmp1              ; Backup coordinates
        sty     tmp2
        jsr     popptr1           ; Pop pointer to string into ptr1

        ldy     #0
:       lda     (ptr1),y
        beq     print_done

        sty     tmp3              ; Backup string walker
        ldx     tmp1              ; Reload X coordinate
        ldy     tmp2              ; Reload Y coordinate
        jsr     _print_char       ; Print the character
        inc     tmp1              ; Increment X
        inc     tmp3              ; Increment string walker
        ldy     tmp3
        bne     :-

print_done:
        ldx     tmp1              ; Reload X coordinate
        ldy     tmp2              ; Reload Y coordinate
        rts
.endproc

; X: start X coordinate
; Y: bottom Y coordinate
; A: Maximum string length
.proc _read_string
        stx     tmp1              ; Remember X coordinate
        sty     put_char+1        ; Remember Y coordinate
        sta     max_str_len+1     ; And max string length
        ldx     #$00
        stx     tmp2              ; String len

        lda     #$00
        sta     _str_input,x      ; Terminate the string

        bit     KBDSTRB
get_key:
        ldx     tmp1
        ldy     put_char+1
        lda     #'_'
        jsr     _print_char
        lda     KBD
        bpl     get_key
        bit     KBDSTRB

        and     #$7F
        cmp     #$0D              ; Is it enter?
        beq     validate
        cmp     #$08              ; Is it left arrow?
        beq     delete_char
        cmp     #$7F              ; Or backspace?
        beq     delete_char
        cmp     #' '              ; We only take space to Z
        bcc     get_key

        cmp     #'a'              ; Check if lower-case
        bcc     :+
        sbc     #$20              ; In this case, upper-case it (carry is set)

:       cmp     #('Z'+1)
        bcs     get_key

        ; We can print that letter
        ldx     tmp2
max_str_len:
        cpx     #$FF               ; Only N chars max, otherwise wait for Enter
        bcs     get_key

        ldx     tmp2
        sta     _str_input,x      ; Save char to string

        ldx     tmp1
        inc     tmp1
        inc     tmp2
put_char:
        ldy     #$FF
        jsr     _print_char
        ldx     tmp2
        lda     #$00
        sta     _str_input,x      ; Terminate the string
        jmp     get_key

delete_char:
        ldx     tmp2
        beq     get_key

        ; Remove underscore
        ldx     tmp1
        ldy     put_char+1
        lda     #' '
        jsr     _print_char

        dec     tmp2
        dec     tmp1
        ldx     tmp1
        lda     #' '
        jmp     put_char

validate:
        rts
.endproc

        .bss

_str_input: .res 32
