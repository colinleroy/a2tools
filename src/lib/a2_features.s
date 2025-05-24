;
; Colin Leroy-Mira, 2025-05
;
; Easy-to-test (from C) feature flags
;

        .export         _is_iigs

        .import         ostype
        .constructor    _init_features, 8

        .include        "apple2.inc"

; ------------------------------------------------------------------------

        .segment        "DATA"

_is_iigs: .byte 0

        .segment        "ONCE"

_init_features:
        lda     ostype
        rol                   ; High bit to carry
        rol     _is_iigs      ; carry to flag low bit
        rts
