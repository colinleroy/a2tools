;
; From Apple ][e ROM
; https://ia800708.us.archive.org/35/items/Apple_IIe_Technical_Reference_Manual/Apple_IIe_Technical_Reference_Manual_text.pdf
; pages 366 - 368
;

        .export         FVTABZ
        .importzp       tmp1
        .include        "apple2.inc"
.ifndef AVOID_ROM_CALLS
        .import         VTABZ
.else
        .ifndef __APPLE2ENH__
        .import         machinetype
        .endif
.endif

        .data

.ifdef AVOID_ROM_CALLS

FBASL:
        .byte $00
        .byte $80
        .byte $00
        .byte $80
        .byte $00
        .byte $80
        .byte $00
        .byte $80
        .byte $28
        .byte $A8
        .byte $28
        .byte $A8
        .byte $28
        .byte $A8
        .byte $28
        .byte $A8
        .byte $50
        .byte $D0
        .byte $50
        .byte $D0
        .byte $50
        .byte $D0
        .byte $50
        .byte $D0

FBASH:
        .byte $04
        .byte $04
        .byte $05
        .byte $05
        .byte $06
        .byte $06
        .byte $07
        .byte $07
        .byte $04
        .byte $04
        .byte $05
        .byte $05
        .byte $06
        .byte $06
        .byte $07
        .byte $07
        .byte $04
        .byte $04
        .byte $05
        .byte $05
        .byte $06
        .byte $06
        .byte $07
        .byte $07

        .bss

NLINES: .res 1

        .code

FVTABZ:
        stx     tmp1

        cmp     #24
        bcc     :+
        lda     #23

:       tax
        lda     FBASH,x
        sta     BASH
        lda     WNDLFT
        .ifndef __APPLE2ENH__
        bit     machinetype
        bpl     colforty
        .endif
        bit     RD80VID
        bpl     colforty
        lsr     a
        clc
colforty:
        adc     FBASL,x
        sta     BASL
        ldx     tmp1
        rts

.else
FVTABZ = VTABZ
.endif
