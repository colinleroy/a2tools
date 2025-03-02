        .export    _print_char
        .export    _print_string

        .import    _font, _font_mask
        .import    _hgr_hi, _hgr_low

        .import    popptr1
        .importzp  ptr1, tmp1, tmp2, tmp3

        .include   "apple2.inc"
        .include   "font.gen.inc"

; A: ASCII code of the char
; X: X coord / 7
; Y: bottom Y coord
_print_char:
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

; TOS: Address of the buffer to print
; X: X coord / 7
; Y: bottom Y coord
; Returns with updated X
_print_string:
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
