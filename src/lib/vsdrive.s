
                .import         _serial_read_byte_no_irq
                .import         serial_read_byte_no_irq_timeout
                .import         _serial_putc_direct
                .import         throbber_on, throbber_off
                .import         _serial_shorten_timeout

                .export         _vsdrive_install, _vsdrive_uninstall
                .destructor     _vsdrive_uninstall, 9

                .include "apple2.inc"

; PRODOS DEVICE DRIVERS ADDRESSES
DEVADR01            = $BF10
DEVADR02            = $BF20

; PRODOS DEVICE COUNT/LIST
DEVCNT              = $BF31
DEVLST              = $BF32
; PRODOS DATE/TIME STORAGE
DATE                = $BF90
TIME                = $BF92

; PRODOS ZERO PAGE VALUES
COMMAND             = $42 ; PRODOS COMMAND
UNIT                = $43 ; PRODOS SLOT/DRIVE
BUFLO               = $44 ; LOW BUFFER
BUFHI               = $45 ; HI BUFFER
BLKLO               = $46 ; LOW REQUESTED BLOCK
BLKHI               = $47 ; HI REQUESTED BLOCK

; PRODOS ERROR CODES
IOERR               = $27
NODEV               = $28
SUCCESS             = $00

; PRODOS IO COMMANDS
PRODOS_GETSTAT_CMD  = $00
PRODOS_READBLK_CMD  = $01
PRODOS_WRITEBLK_CMD = $02

; VSDrive command/control codes
VD_ENVELOPE_CMD     = $C5
VD_WRITEBLK_CMD     = $02
VD_READBLK_CMD      = $03

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
        lda       DEVLST,X                    ; Grab an active device number
        cmp       VS_SLOT_DEV1                ; Slot x, drive 1?
        beq       scanslots                   ; Yes, someone already home - go to next slot
        cmp       VS_SLOT_DEV2                ; Slot x, drive 2?
        beq       scanslots                   ; Yes, someone already home - go to next slot
        dex
        bpl       checkdev                    ; Swing around until no more in list

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
        lda       VS_SLOT_DEV1                ; Slot x, drive 1
        sta       DEVLST,Y
        inc       DEVCNT
        iny
        lda       VS_SLOT_DEV2                ; Slot x, drive 2
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
        lda       #$00                        ; Unit number (0 or 2)
        ldx       UNIT
        cpx       VS_SLOT_DEV1
        beq       do_command
        cpx       VS_SLOT_DEV2
        beq       do_drive2

        lda       #NODEV                      ; Fail, not a command for our drives
        sec
        rts

do_drive2:
        lda       #$02                        ; Unit number 2
do_command:
        ldx       COMMAND
        beq       GETSTAT
        cpx       #PRODOS_READBLK_CMD
        beq       READBLK
        cpx       #PRODOS_WRITEBLK_CMD
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

READBLK:
        clc
        adc        #VD_READBLK_CMD            ; Command will be 3 for unit 0, 5 for unit 2
        sta        CURCMD
        ; SEND COMMAND TO PC
        jsr        COMMAND_ENVELOPE

        ; Shorten timeout
        lda        _serial_shorten_timeout
        pha
        lda        #$C0
        sta        _serial_shorten_timeout
        ; Pull and verify command envelope from host
        jsr        serial_read_byte_no_irq_timeout
        bcs        IOFAIL
        cmp        #VD_ENVELOPE_CMD
        bne        IOFAIL
        jsr        _serial_read_byte_no_irq   ; Read command
        cmp        CURCMD
        bne        IOFAIL
        jsr        _serial_read_byte_no_irq   ; LSB of requested block
        cmp        BLKLO
        bne        IOFAIL
        jsr        _serial_read_byte_no_irq   ; MSB of requested block
        cmp        BLKHI
        bne        IOFAIL

        ldx        #$00
:       jsr        _serial_read_byte_no_irq   ; Four bytes of time/date
        sta        TEMPDT,x
        eor        CHECKSUM
        sta        CHECKSUM
        inx
        cpx        #$04
        bcc        :-

        jsr        _serial_read_byte_no_irq   ; Checksum of command envelope
        cmp        CHECKSUM
        bne        IOFAIL

        lda        TEMPDT
        sta        TIME
        lda        TEMPDT+1
        sta        TIME+1
        lda        TEMPDT+2
        sta        DATE
        lda        TEMPDT+3
        sta        DATE+1

        ; READ BLOCK AND VERIFY
        ldx        #$02
        ldy        #$00
        sty        CHECKSUM
RDLOOP:
        jsr        _serial_read_byte_no_irq
        sta        (BUFLO),Y
        eor        CHECKSUM
        sta        CHECKSUM
        iny
        bne        RDLOOP

        inc        BUFHI
        dex
        bne        RDLOOP

        dec        BUFHI
        dec        BUFHI                      ; Bring BUFHI back down to where it belongs

        jsr        _serial_read_byte_no_irq   ; Block checksum
        cmp        CHECKSUM
        beq        IOSUCCESS

IOFAIL:
        jsr        throbber_off
        pla
        sta        _serial_shorten_timeout
        lda        #IOERR
        sec
        rts

WRITEBLK:
        ; SEND COMMAND TO PC
        clc
        adc        #VD_WRITEBLK_CMD           ; Command will be 2 for unit 0 or 4 for unit 2
        sta        CURCMD
        jsr        COMMAND_ENVELOPE

        ; WRITE BLOCK AND CHECKSUM
        ldx        #$02
        ldy        #$00
        sty        CHECKSUM
WRLOOP:
        lda        (BUFLO),Y
        jsr        SEND_AND_CHECKSUM
        iny
        bne        WRLOOP

        inc        BUFHI
        dex
        bne        WRLOOP

        dec        BUFHI
        dec        BUFHI

        lda        CHECKSUM                   ; Send checksum
        jsr        _serial_putc_direct

        lda        _serial_shorten_timeout
        pha
        lda        #$C0
        sta        _serial_shorten_timeout
        ; READ ECHO'D COMMAND AND VERIFY
        jsr        serial_read_byte_no_irq_timeout
        bcs        IOFAIL
        cmp        #VD_ENVELOPE_CMD           ; S/B Command envelope
        bne        IOFAIL
        jsr        _serial_read_byte_no_irq
        cmp        CURCMD                     ; S/B Write
        bne        IOFAIL
        jsr        _serial_read_byte_no_irq   ; Read LSB of requested block
        cmp        BLKLO
        bne        IOFAIL
        jsr        _serial_read_byte_no_irq   ; Read MSB of requested block
        cmp        BLKHI
        bne        IOFAIL
        jsr        _serial_read_byte_no_irq   ; Checksum of block - not the command envelope
        cmp        CHECKSUM
        bne        IOFAIL
IOSUCCESS:
        jsr        throbber_off
        pla
        sta        _serial_shorten_timeout
        lda        #SUCCESS
        clc
        rts

        ; And done!

COMMAND_ENVELOPE:
        ; Send a command envelope (read/write) with the command in the accumulator
        pha                                   ; Hang on to the command for a sec...
        lda        #$00
        sta        CHECKSUM
        lda        #VD_ENVELOPE_CMD
        jsr        SEND_AND_CHECKSUM          ; Envelope
        pla                                   ; Pull the command back off the stack
        jsr        SEND_AND_CHECKSUM          ; Send command
        lda        BLKLO
        jsr        SEND_AND_CHECKSUM          ; Send LSB of requested block
        lda        BLKHI
        jsr        SEND_AND_CHECKSUM          ; Send MSB of requested block
        jsr        _serial_putc_direct        ; Send envelope checksum (already in A after SEND_AND_CHECKSUM)
        jmp        throbber_on                ; Turn indicator on

SEND_AND_CHECKSUM:
        jsr        _serial_putc_direct        ; Send command
        eor        CHECKSUM
        sta        CHECKSUM
        rts

                  .data

VS_SLOT:          .byte $01
VS_SLOT_DEV1:     .byte $00
VS_SLOT_DEV2:     .byte $00
UNIT2:            .byte $00
CHECKSUM:         .byte $00
TEMPDT:           .byte $00,$00,$00,$00
CURCMD:           .byte  $00
original_driver:  .byte $00,$00
installed:        .byte $00
slotcnt:          .byte $00
