
                .export _clreol, _clrzone
                .import FVTABZ, cputdirect
                .import incsp3
                .importzp sp
                .include "apple2.inc"

_clreol:
        lda     CH
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
        sta     clr_lxe

        jsr     incsp3

update_screen:
        lda     clr_lys
        sta     CV

next_line:
        jsr     FVTABZ
        lda     clr_lxs
        sta     CH

:       lda     #' '|$80
        jsr     cputdirect

        lda     CH
        cmp     clr_lxe
        bcc     :-

        ; Last X (backup CV in case it's last column of scrollwindow)
        lda     CV
        pha
        lda     #' '|$80
        jsr     cputdirect
        pla
        .ifdef __APPLE2ENH__
        inc     a
        .else
        clc
        adc     #1
        .endif
        sta     CV
        cmp     clr_lye
        bcc     next_line

        ; gotoxy top-left
        pla
        sta     CV
        jsr     FVTABZ
        pla
        sta     CH
        rts
        .bss

clr_lxs:      .res 1
clr_lxe:      .res 1
clr_lys:      .res 1
clr_lye:      .res 1
