        .export       _atoc
        .import       _atoi

; unsigned char __fastcall__ atoc(int x)
_atoc:
        jsr       _atoi
        cpx       #$00
        bne       :+
        rts
:
        lda       #$FF
        ldx       #$00
        rts
