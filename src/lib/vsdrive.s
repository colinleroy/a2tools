
                .import         _serial_read_byte_no_irq
                .import         serial_read_byte_no_irq_timeout
                .import         _serial_putc_direct
                .export         _vsdrive_install, _vsdrive_uninstall
                .destructor     _vsdrive_uninstall, 9

                .include "apple2.inc"

; PRODOS DEVICE DRIVERS ADDRESSES
DEVADR01        = $BF10
DEVADR02        = $BF20
; PRODOS DEVICE COUNT/LIST
DEVCNT    = $BF31
DEVLST    = $BF32

; PRODOS ZERO PAGE VALUES
COMMAND   = $42 ; PRODOS COMMAND
UNIT      = $43 ; PRODOS SLOT/DRIVE
BUFLO     = $44 ; LOW BUFFER
BUFHI     = $45 ; HI BUFFER
BLKLO     = $46 ; LOW REQUESTED BLOCK
BLKHI     = $47 ; HI REQUESTED BLOCK

; PRODOS ERROR CODES
IOERR        = $27
NODEV        = $28
WPERR        = $2B

DATE         = $BF90 ; Date storage
TIME         = $BF92 ; Time storage

SCRN_THROB   = $0427

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

_vsdrive_uninstall:
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
        sta       installed
        sta       slotcnt
        sta       DEVLST,y
        dey
        sta       DEVLST,y
        dey
        sty       DEVCNT
uninstall_done:
        rts

vsdrive_driver:
        cld
        lda       UNIT
        cmp       VS_SLOT_DEV1
        beq       do_drive1
        cmp       VS_SLOT_DEV2
        beq       do_drive2
        lda       #NODEV
vsdrive_driver_fail:
        sec
        rts
do_drive1:
        lda       #$00
        sta       UNIT2
        beq       do_command
do_drive2:
        lda       #$02
        sta       UNIT2
do_command:
        lda       COMMAND
        beq       GETSTAT
        cmp       #$01
        bne       :+
        jmp       READBLK
:       cmp       #$02
        bne       :+
        jmp       WRITEBLK
:       lda       #$00
        clc
        rts

GETSTAT:
        lda       #$00
        ldx       #$FF
        ldy       #$FF
        clc
        rts

; Calculate the checksum of the block at BUFLO/BUFHI
CALC_CHECKSUM:
        lda       #$00        ; Clean everyone out
        tax
        tay
        CC_LOOP:
        eor       (BUFLO),Y   ; Exclusive-or accumulator with what's at (BUFLO),Y
        sta       CHECKSUM    ; Save that tally in CHECKSUM as we go
        iny
        bne       CC_LOOP
        inc       BUFHI       ; Y just turned over to zero; bump MSB of buffer
        inx                   ; Keep track of trips through the loop - we need two of them
        cpx       #$02        ; The second time X is incremented, this will signfiy twice through the loop
        bne       CC_LOOP

        dec        BUFHI             ; BUFHI got bumped twice, so back it back down
        dec        BUFHI
        rts

READFAIL:
        lda        SCREEN_CONTENTS        ; Restore screen contents
        sta        SCRN_THROB
        lda        #$01
        sec
        rts

READBLK:
        lda        #$03               ; Read command w/time request - command will be either 3 or 5
        clc
        adc        UNIT2              ; Command will be #$05 for unit 2
        sta        CURCMD
; SEND COMMAND TO PC
        jsr        COMMAND_ENVELOPE
; Grab the screen contents, remember it
        lda        SCRN_THROB
        sta        SCREEN_CONTENTS
; Pull and verify command envelope from host
        jsr        serial_read_byte_no_irq_timeout        ; Command envelope begin
        bcs        READFAIL
        cmp        #$C5
        bne        READFAIL
        jsr        _serial_read_byte_no_irq               ; Read command
        cmp        CURCMD
        bne        READFAIL
        jsr        _serial_read_byte_no_irq               ; LSB of requested block
        cmp        BLKLO
        bne        READFAIL
        jsr        _serial_read_byte_no_irq               ; MSB of requested block
        cmp        BLKHI
        bne        READFAIL
        jsr        _serial_read_byte_no_irq               ; LSB of time
        sta        TEMPDT
        eor        CHECKSUM
        sta        CHECKSUM
        jsr        _serial_read_byte_no_irq               ; MSB of time
        sta        TEMPDT+1
        eor        CHECKSUM
        sta        CHECKSUM
        jsr        _serial_read_byte_no_irq               ; LSB of date
        sta        TEMPDT+2
        eor        CHECKSUM
        sta        CHECKSUM
        jsr        _serial_read_byte_no_irq               ; MSB of date
        sta        TEMPDT+3
        eor        CHECKSUM
        sta        CHECKSUM
        jsr        _serial_read_byte_no_irq               ; Checksum of command envelope
        cmp        CHECKSUM
        bne        WRITEFAIL          ; Just need a nearby failure
        lda        TEMPDT
        sta        TIME
        lda        TEMPDT+1
        sta        TIME+1
        lda        TEMPDT+2
        sta        DATE
        lda        TEMPDT+3
        sta        DATE+1
; READ BLOCK AND VERIFY
        ldx        #$00
        ldy        #$00
        stx        SCRN_THROB
RDLOOP:
        jsr        _serial_read_byte_no_irq
        sta        (BUFLO),Y
        iny
        bne        RDLOOP

        inc        BUFHI
        inx
        stx        SCRN_THROB
        cpx        #$02
        bne        RDLOOP

        dec        BUFHI
        dec        BUFHI        ; Bring BUFHI back down to where it belongs

        lda        SCREEN_CONTENTS        ; Restore screen contents
        sta        SCRN_THROB

        jsr        _serial_read_byte_no_irq        ; Checksum
        pha                ; Push checksum for now
        ldx        #$00
        jsr        CALC_CHECKSUM
        pla        
        cmp        CHECKSUM
        bne        WRITEFAIL        ; Just need a failure exit nearby

        lda        #$00
        clc
        rts

WRITEFAIL:
        lda        SCREEN_CONTENTS        ; Restore screen contents
        sta        SCRN_THROB
        lda        #$01
        sec
        rts

; WRITE
WRITEBLK:
; SEND COMMAND TO PC
        lda        #$02                ; Write command - command will be either 2 or 4
        clc
        adc        UNIT2                ; Command will be #$05 for unit 2
        sta        CURCMD
        jsr        COMMAND_ENVELOPE

; WRITE BLOCK AND CHECKSUM
        ldx        #$00
        stx        CHECKSUM
WRBKLOOP:
        ldy        #$00
WRLOOP:
        lda        (BUFLO),Y
; Write screen throbber
        sta        SCRN_THROB
        jsr        _serial_putc_direct
        iny
        bne        WRLOOP

        inc        BUFHI
        inx
        cpx        #$02
        bne        WRBKLOOP

        dec        BUFHI
        dec        BUFHI

        jsr        CALC_CHECKSUM
        lda        CHECKSUM        ; Checksum
        jsr        _serial_putc_direct

; READ ECHO'D COMMAND AND VERIFY
        jsr        serial_read_byte_no_irq_timeout
        bcs        WRITEFAIL
        cmp        #$C5                ; S/B Command envelope
        bne        WRITEFAIL
        jsr        _serial_read_byte_no_irq
        cmp        CURCMD                ; S/B Write
        bne        WRITEFAIL
        jsr        _serial_read_byte_no_irq                ; Read LSB of requested block
        cmp        BLKLO
        bne        WRITEFAIL
        jsr        _serial_read_byte_no_irq                ; Read MSB of requested block
        cmp        BLKHI
        bne        WRITEFAIL
        jsr        _serial_read_byte_no_irq                ; Checksum of block - not the command envelope
        cmp        CHECKSUM
        bne        WRITEFAIL
        lda        #$00
        clc
        rts

COMMAND_ENVELOPE:
                ; Send a command envelope (read/write) with the command in the accumulator
        pha                        ; Hang on to the command for a sec...
        lda        #$C5
        jsr        _serial_putc_direct                ; Envelope
        sta        CHECKSUM
        pla                        ; Pull the command back off the stack
        jsr        _serial_putc_direct                ; Send command
        eor        CHECKSUM
        sta        CHECKSUM
        lda        BLKLO
        jsr        _serial_putc_direct                ; Send LSB of requested block
        eor        CHECKSUM
        sta        CHECKSUM
        lda        BLKHI
        jsr        _serial_putc_direct                ; Send MSB of requested block
        eor        CHECKSUM
        sta        CHECKSUM
        jsr        _serial_putc_direct                ; Send envelope checksum
        rts


                  .data

VS_SLOT:          .byte $01
VS_SLOT_DEV1:     .byte $00
VS_SLOT_DEV2:     .byte $00
UNIT2:            .byte $00
CHECKSUM:         .byte $00
SCREEN_CONTENTS:  .byte $00
TEMPDT:           .byte $00,$00,$00,$00
CURCMD:           .byte  $00
original_driver:  .byte $00,$00
installed:        .byte $00
slotcnt:          .byte $00
