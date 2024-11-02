
                .importzp       sp
                .export         _vsdrive_install
                .destructor     vsdrive_uninstall, 9

                .include "apple2.inc"

DEVADR01	= $BF10
DEVADR02	= $BF20
DEVCNT    = $BF31
DEVLST    = $BF32

                .segment "RT_ONCE"

_vsdrive_install:
        lda       installed
        bne       install_done
; Find a likely place to install the driver in the device list.
; Is there already a driver in slot x, drive 1?
scanslots:
        inc       slotcnt
        lda       slotcnt
        sta       VS_SLOT
        cmp       #$08
        beq       install_done
        asl
        asl
        asl
        asl
        sta       VS_SLOT_DEV1
        clc
        adc       #$80
        sta       VS_SLOT_DEV2
        ldx       DEVCNT
checkdev:
        lda       DEVLST,X ; Grab an active device number
        cmp       VS_SLOT_DEV1 ; Slot x, drive 1?
        beq       scanslots ; Yes, someone already home - go to next slot
        cmp       VS_SLOT_DEV2 ; Slot x, drive 2?
        beq       scanslots ; Yes, someone already home - go to next slot
        dex
        bpl       checkdev ; Swing around until no more in list

        ; We have a slot!
        lda       VS_SLOT
        asl
        tax
        ; Backup previous driver address
        lda       DEVADR01,x
        sta       original_driver
        lda       #<vsdrive_driver
        sta       DEVADR01,x
        sta       DEVADR02,x

        inx
        lda       DEVADR01,x
        sta       original_driver+1
        lda       #>vsdrive_driver
        sta       DEVADR01,x
        sta       DEVADR02,x
        ; Add to device list
        inc       DEVCNT
        ldy       DEVCNT
        lda       VS_SLOT_DEV1 ; Slot x, drive 1
        sta       DEVLST,Y
        inc       DEVCNT
        iny
        lda       VS_SLOT_DEV2 ; Slot x, drive 2
        sta       DEVLST,Y
        lda       #1
        sta       installed
install_done:
        rts

                .segment "CODE"

vsdrive_uninstall:
        lda       installed
        beq       uninstall_done
        lda       VS_SLOT
        asl
        tax
        ; Restore previous driver address
        lda       original_driver
        sta       DEVADR01,x
        sta       DEVADR02,x

        inx
        lda       original_driver+1
        sta       DEVADR01,x
        sta       DEVADR02,x

        ldy       DEVCNT
        lda       #$00
        sta       DEVLST,y
        dey
        sta       DEVLST,y
        dey
        sty       DEVCNT
uninstall_done:
        rts

vsdrive_driver:
        sec
        rts

                  .data

VS_SLOT:          .byte $01
VS_SLOT_DEV1:     .byte $00
VS_SLOT_DEV2:     .byte $00
original_driver:  .byte $00,$00
installed:        .byte $00
slotcnt:          .byte $00
