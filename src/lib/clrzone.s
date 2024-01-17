
                .export _clreol, _clrzone
                .import FVTABZ
                .import incsp3
                .importzp sp
                .include "apple2.inc"

_clreol:
        lda     CH
        sta     clr_lxs_even
        clc
        lda     CV
        sta     clr_lys
        sta     clr_lye
        sta     clr_y_bck
        lda     WNDWDTH
        sec
        sbc     #1
        sta     clr_lxe_even
        jmp     update_screen

_clrzone:
        sta     clr_lye

        ldy     #$02
        lda     (sp),y
        sta     clr_lxs_even

        dey
        lda     (sp),y
        sta     clr_lys

        dey
        lda     (sp),y
        sta     clr_lxe_even

        jsr     incsp3

        clc
        lda     WNDTOP
        adc     clr_lys
        sta     clr_lys
        sta     clr_y_bck

        lda     WNDTOP
        adc     clr_lye
        adc     #1
        sta     clr_lye

update_screen:
        bit     RD80VID
        bmi     setup_80_bounds

        lda     clr_lxs_even
        sta     clr_lxs_odd
        sta     clr_x_bck
        lda     clr_lxe_even
        sec
        sbc     clr_lxs_odd
        clc
        adc     #1
        sta     clr_lxe_odd
        lda     #$00
        sta     clr_lxs_even
        sta     clr_lxe_even
        beq     start_update

setup_80_bounds:
        lda     clr_lxs_even
        sta     clr_x_bck
        lsr
        sta     clr_lxs_odd
        bcc     lxs_is_even
        adc     #0

lxs_is_even:
        sta     clr_lxs_even

        clc
        lda     clr_lxe_even
        adc     #1
        lsr
        sta     clr_lxe_odd
        bcc     lxe_is_even
        adc     #0
lxe_is_even:
        sec
        sbc     clr_lxs_even
        sta     clr_lxe_even

        lda     clr_lxe_odd
        sec
        sbc     clr_lxs_odd
        sta     clr_lxe_odd

start_update:
        lda     clr_lys
        sta     CV

next_line:
        jsr     FVTABZ
        pha

        bit     RD80VID
        bpl     do_low

        clc
        adc     clr_lxs_even
        sta     BASL

        lda     #' '|$80

        bit     $C055
        ldy     clr_lxe_even
        beq     do_low
next_char_hi:
        dey
        sta     (BASL),y
        bne     next_char_hi

do_low:
        pla
        clc
        adc     clr_lxs_odd
        sta     BASL

        lda     #' '|$80
        bit     $C054
        ldy     clr_lxe_odd
        beq     do_next_line
next_char_low:
        dey
        sta     (BASL),y
        bne     next_char_low

do_next_line: 
        inc     CV
        lda     CV
        cmp     clr_lye
        bcc     next_line

        lda     clr_x_bck
        sta     CH
        lda     clr_y_bck
        sta     CV
        jmp     FVTABZ

        .bss

clr_lxs_even: .res 1
clr_lxs_odd:  .res 1
clr_lxe_even: .res 1
clr_lxe_odd:  .res 1
clr_lys:      .res 1
clr_lye:      .res 1
clr_x_bck:    .res 1
clr_y_bck:    .res 1
