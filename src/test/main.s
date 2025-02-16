        .export _main

        .import _exit, _platform_msleep
        .import _init_hgr, _init_mouse
        .import  _init_hgr_base_addrs
        .import  _clreol, _cutoa, _cputc, _bzero
        .import  pusha, pushax
        .import  FVTABZ
        .import  _draw
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

        jsr     _init_mouse
        bcs     err

loop:
        ; sei
        ; lda     mouse_x
        ; jsr     pusha
        ; lda     mouse_y
        ; jsr     _draw
        ; cli
        jmp     loop

err:
        jmp     _exit
