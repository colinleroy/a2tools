;
; Colin Leroy-Mira, 2025-05
;
; Easy-to-test (from C) feature flags
;

        .export         _is_iigs, _is_iieenh, _has_80cols

        .import         ostype
        .constructor    _init_features, 8

        .include        "apple2.inc"

; ------------------------------------------------------------------------

        .segment        "DATA"

_is_iigs:     .byte $00
_is_iieenh:   .byte $00

; FIXME: Update once cc65 gets dynamic 80cols feature
.ifdef __APPLE2ENH__
_has_80cols: .byte $80
.else
_has_80cols: .byte $00
.endif

        .segment        "ONCE"

_init_features:
        lda     ostype

        ; IIe Enhanced? (means MouseText is there)
        cmp     #$31
        ror     _is_iieenh    ; Carry to flag high bit

        ; IIgs ?
        rol                   ; High bit to carry
        ror     _is_iigs      ; carry to flag high bit
        rts
