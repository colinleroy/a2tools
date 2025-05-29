;
; Colin Leroy-Mira, 2025-05
;
; Easy-to-test (from C) feature flags
;

        .export         _is_iigs, _is_iie, _is_iieenh
        .export         _has_80cols, _has_128k
        .export         _try_videomode

        .ifdef __APPLE2ENH__
        .export         _CH_VLINE ; Need that for file_select
        .endif

        .import         ostype, _videomode, _allow_lowercase
        .constructor    _init_features, 8

        .include        "apple2.inc"

MACHID := $BF98
; ------------------------------------------------------------------------

        .segment        "DATA"

_is_iigs:     .byte $00
_is_iie:      .byte $00
_is_iieenh:   .byte $00
_has_80cols:  .byte $00
_has_128k:    .byte $00

.ifdef __APPLE2ENH__
_CH_VLINE:    .byte $DF
.endif

        .segment "ONCE"

_init_features:
        lda     ostype

        ; IIe? (means 80cols can be there, and Apple keys)
        cmp     #$30
        bcc     :+
        ror     _is_iie       ; Carry to flag high bit
        pha
        lda     #1
        jsr     _allow_lowercase
        pla

:       ; IIe Enhanced? (means MouseText is there)
        cmp     #$31
        ror     _is_iieenh    ; Carry to flag high bit

        ; IIgs ?
        rol                   ; High bit to carry
        ror     _is_iigs      ; carry to flag high bit

        lda     MACHID        ; 128k?
        and     #$30
        cmp     #$30
        bne     :+

        lda     #$FF
        sta     _has_128k

:       rts

        .segment "CODE"

.proc _try_videomode
        ldy     #$FF          ; Consider we succeed
        sty     _has_80cols

        jsr     _videomode
        cmp     #$FF
        bne     :+
        inc     _has_80cols   ; We did not succeed.
:       rts
.endproc
