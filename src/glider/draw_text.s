        .export   _print_char

        .import   _font, _font_mask
        .import   _hgr_hi, _hgr_low

        .importzp  tmp4

        .include   "apple2.inc"
        .include   "font.gen.inc"

; A: number of the char
; X: X coord / 7
; Y: bottom Y coord
_print_char:
        stx     tmp4
        asl
        tax
        lda     _font,x
        sta     char_data+1
        lda     _font+1,x
        sta     char_data+2
        
        ldx     #4
        clc

:       lda     _hgr_low,y
        adc     tmp4
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
