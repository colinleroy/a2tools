        .export       _hgr_hi, _hgr_low
        .export       _mod7_table, _div7_table
        .export       _build_hgr_tables
        .importzp     tmp1, tmp2

HGR_HEIGHT = 192

_build_hgr_tables:
        ldy     #$00

next_y:
        ; ABCDEFGH -> pppFGHCD EABAB000
        ; CD(E)
        tya
        and     #%00111000
        lsr
        lsr
        lsr
        lsr
        sta     tmp2

        ; EABAB
        tya
        and     #%11000000
        ror                   ; E
        sta     tmp1
        and     #%01100000
        lsr
        lsr
        ora     tmp1
        sta     _hgr_low,y

        ; FGH
        tya
        and     #%00000111
        asl
        asl
        ora     tmp2

        ; ppp
        ora     #$20         ; first page
        sta     _hgr_hi,y

        iny
        cpy     #HGR_HEIGHT
        bne     next_y

build_div7_tables:
        ldx     #0
        ldy     #0
        sty     tmp1

:       lda     tmp1
        sta     _div7_table,x

        tya
        sta     _mod7_table,x

        inx                   ; Are we done?
        beq     :+
        iny
        cpy     #7
        bne     :-

        ldy     #0
        inc     tmp1
        bne     :-

:       rts

.bss

_hgr_hi:        .res 192
_hgr_low:       .res 192
_div7_table:    .res 256
_mod7_table:    .res 256
