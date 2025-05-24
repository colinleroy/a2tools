;
; Colin Leroy-Mira, 2025-05
;
; Easy-to-test (from C) feature flags
;

        .export         _is_iigs, _is_iie, _is_iieenh, _has_80cols
        .export         _try_videomode

        .export         machinetype ; FIXME remove once in cc65
        .export         CH_VLINE    ; FIXME id

        .import         ostype, _videomode
        .constructor    _init_features, 8

        .include        "apple2.inc"

; ------------------------------------------------------------------------

        .segment        "DATA"

_is_iigs:     .byte $00
_is_iie:      .byte $00
_is_iieenh:   .byte $00
_has_80cols:  .byte $00

; FIXME: Update once cc65 gets dynamic 80cols feature
.ifdef __APPLE2ENH__

; bit 7: Machine is a //e or newer
; bit 6: Machine is a //e enhanced or newer
machinetype: .byte %11000000
CH_VLINE:    .byte $DF

.else

machinetype: .byte $00
CH_VLINE:    .byte '!'

.endif

        .segment "ONCE"

_init_features:
        lda     ostype

        ; IIe? (means 80cols can be there, and Apple keys)
        cmp     #$30
        ror     _is_iie       ; Carry to flag high bit

        ; IIe Enhanced? (means MouseText is there)
        cmp     #$31
        ror     _is_iieenh    ; Carry to flag high bit

        ; IIgs ?
        rol                   ; High bit to carry
        ror     _is_iigs      ; carry to flag high bit
        rts

        .segment "CODE"

.proc _try_videomode
        ldy     #$00
        cmp     #$15
        beq     :+
        ldy     #$FF
:       sty     _has_80cols

.ifdef __APPLE2ENH__
        jsr     _videomode
.else
        lda     #$FF            ; FIXME pretend we failed
.endif
        cmp     #$FF
        bne     :+
        eor     _has_80cols
        sta     _has_80cols
:       rts
.endproc
