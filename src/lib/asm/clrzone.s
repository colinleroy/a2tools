
                .export _clreol, _clrzone
                .import FVTABZ, cputdirect, machinetype
                .import incsp3
                .importzp sp
                .include "apple2.inc"

        .segment "LOWCODE"

_clreol:
        lda     CH

        bit     machinetype
        bpl     :+
        bit     RD80VID
        bpl     :+
        lda     OURCH
:

        sta     clr_lxs
        pha                     ; Backup start X
        lda     CV
        sta     clr_lys
        sta     clr_lye
        pha                     ; Backup start Y
        lda     WNDWDTH
        sec
        sbc     #1
        sta     clr_lxe
        jmp     update_screen

_clrzone:
        sec                     ; Set carry to inc by one
        adc     WNDTOP
        sta     clr_lye

        ldy     #$02
        lda     (sp),y
        sta     clr_lxs
        pha                     ; backup start X

        dey
        lda     (sp),y
        clc
        adc     WNDTOP
        sta     clr_lys
        pha                     ; backup start Y

        dey
        lda     (sp),y
        cmp     WNDWDTH         ; Make sure end bound is sane
        bcc     :+
        ldy     WNDWDTH
        dey
        tya
:       sta     clr_lxe

        jsr     incsp3

update_screen:
        lda     clr_lys
        sta     CV

next_line:
        jsr     FVTABZ
        lda     clr_lxs
        bit     machinetype
        bpl     :+
        sta     OURCH
:       sta     CH

        ; (backup CV in case it's last column of scrollwindow)
        lda     CV
        pha

next_char:
        lda     #' '|$80
        jsr     cputdirect

        bit     machinetype
        bpl     get40
        bit     RD80VID
        bpl     get40
        lda     OURCH
        jmp     cmp_char
get40:
        lda     CH
cmp_char:
        beq     :+            ; If 0, we wrapped. Go check if last line.
        cmp     clr_lxe
        bcc     next_char     ; last X?
        beq     next_char

:       pla
        clc
        adc     #1
        sta     CV
        cmp     clr_lye
        bcc     next_line

        ; gotoxy top-left
        pla
        sta     CV
        jsr     FVTABZ
        pla
        bit     machinetype
        bpl     :+
        sta     OURCH
:       sta     CH
        rts
        .bss

clr_lxs:      .res 1
clr_lxe:      .res 1
clr_lys:      .res 1
clr_lye:      .res 1
