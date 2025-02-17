        .export _main

        .export _hgr_low, _hgr_hi

        .import _exit, _platform_msleep
        .import _init_hgr, _init_mouse
        .import  _init_hgr_base_addrs, _hgr_baseaddr
        .import  _clreol, _cutoa, _cputc, _bzero
        .import  pusha, pushax
        .import  FVTABZ
        .import  mouse_x, mouse_y
        
        .include "apple2.inc"

_main:

        lda     #<$2000
        ldx     #>$2000
        jsr     pushax

        lda     #<$4000
        ldx     #>$4000
        jsr     _bzero

        lda     #1
        jsr     _init_hgr

        jsr     _init_hgr_base_addrs

        jsr     _init_simple_hgr_addrs

        jsr     _init_mouse
        bcs     err

loop:
        jmp     loop

err:
        jmp     _exit

; Copy the hgr_baseaddr array of addresses
; to two arrays of low bytes/high bytes for simplicity
_init_simple_hgr_addrs:
        ldy     #0
        ldx     #0
:       lda     _hgr_baseaddr,x
        sta     _hgr_low,y
        inx
        lda     _hgr_baseaddr,x
        sta     _hgr_hi,y
        iny
        inx
        bne     :-

:       lda     _hgr_baseaddr+256,x
        sta     _hgr_low,y
        inx
        lda     _hgr_baseaddr+256,x
        sta     _hgr_hi,y
        inx
        iny
        cpy     #192
        bne     :-

        rts

        .bss

_hgr_low: .res 192
_hgr_hi:  .res 192
