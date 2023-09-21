;
; Oliver Schmidt, 2012-11-17
; Colin Leroy-Mira, 2023-09-02 - Handle IRQs without ProDOS
;
; IRQ handling (Apple2gs version)
;

        .setcpu         "65816"

        .export         _init_fast_irq, _done_fast_irq
        .import         callirq
        .constructor    _init_fast_irq, 9
        .destructor     _done_fast_irq, 9

        .include        "apple2.inc"

; ------------------------------------------------------------------------


        .segment       "DATA"

Emulate:       .res 1
OrgMgr:        .res 4

        .segment        "LOWCODE"

_init_fast_irq:
        sei
        clc
        xce
        rep     #$30              ; 16 bits registers
        .i16

        pea     $0000
        pea     $0000
        pea     $0004             ; IntMgr refnum
        ldx     #$1103            ; Get IntMgr vector
        jsl     $E10000

        pla                       ; Save address
        sta     OrgMgr
        pla
        sta     OrgMgr+2

        pea     $0004
        pea     $0000
        pea     handle_ram_irq

        ldx     #$1003            ; Set Vector
        jsl     $E10000

        sec
        xce
        sep     #$30
        .i8
        cli
        rts

; ------------------------------------------------------------------------

_done_fast_irq:
        rts

; ------------------------------------------------------------------------

handle_ram_irq:
        clc
        xce
        bcc     Native

        sec
        xce
        
        php
        pha
        phx
        phy
        
        lda     #$FF
        sta     Emulate
        bra     HandleIrq

Native:
        php

        rep     #$30
        .i16

        pha
        phx
        phy
        phb
        
        pea     $0000
        plb
        plb
        
        sep     #$30
        .i8

        stz     Emulate

HandleIrq:
        jsr     callirq
        bcc     Fallback        ; We did not handle that.

Done:
        bit     Emulate
        bmi     Restore

        rep     #$30
        .i16
        plb

Restore:
        ply
        plx
        pla
        plp
        rti

Fallback:
        bit     Emulate
        bmi     RestoreAndJump

        rep     #$30
        .i16
        plb

RestoreAndJump:
        ply
        plx
        pla
        plp
        jml    (OrgMgr)
