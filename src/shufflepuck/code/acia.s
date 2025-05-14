;
; Serial driver for the Apple II Super Serial Card.
;
; Oliver Schmidt, 21.04.2005
;
; The driver is based on the cc65 rs232 module, which in turn is based on
; Craig Bruce device driver for the Switftlink/Turbo-232.
;
; SwiftLink/Turbo-232 v0.90 device driver, by Craig Bruce, 14-Apr-1998.
;
; This software is Public Domain.  It is in Buddy assembler format.
;
; This device driver uses the SwiftLink RS-232 Serial Cartridge, available from
; Creative Micro Designs, Inc, and also supports the extensions of the Turbo232
; Serial Cartridge.  Both devices are based on the 6551 ACIA chip.  It also
; supports the "hacked" SwiftLink with a 1.8432 MHz crystal.
;
; The code assumes that the kernal + I/O are in context.  On the C128, call
; it from Bank 15.  On the C64, don't flip out the Kernal unless a suitable
; NMI catcher is put into the RAM under then Kernal.  For the SuperCPU, the
; interrupt handling assumes that the 65816 is in 6502-emulation mode.
;

        .export         _acia_open, _acia_close
        .export         _acia_put, _acia_get

        .include        "zeropage.inc"
        .include        "ser-kernel.inc"
        .include        "ser-error.inc"

Offset          = $8F           ; Move 6502 false read out of I/O to page $BF
ACIA           := $C088-Offset

ACIA_DATA      := ACIA+0        ; Data register
ACIA_STATUS    := ACIA+1        ; Status register
ACIA_CMD       := ACIA+2        ; Command register
ACIA_CTRL      := ACIA+3        ; Control register

SLTROMSEL      := $C02D         ; For Apple IIgs slot verification

;----------------------------------------------------------------------------
; Global variables

.segment "s"

Index:          .res    1       ; I/O register index
Slot:           .res    1
Speed:          .res    1

BaudTable:                      ; Table used to translate RS232 baudrate param
                                ; into control register value
                                ; bit7 = 1 means setting is invalid
        .byte   $FF             ; SER_BAUD_45_5
        .byte   $01             ; SER_BAUD_50
        .byte   $02             ; SER_BAUD_75
        .byte   $03             ; SER_BAUD_110
        .byte   $04             ; SER_BAUD_134_5
        .byte   $05             ; SER_BAUD_150
        .byte   $06             ; SER_BAUD_300
        .byte   $07             ; SER_BAUD_600
        .byte   $08             ; SER_BAUD_1200
        .byte   $09             ; SER_BAUD_1800
        .byte   $0A             ; SER_BAUD_2400
        .byte   $0B             ; SER_BAUD_3600
        .byte   $0C             ; SER_BAUD_4800
        .byte   $0D             ; SER_BAUD_7200
        .byte   $0E             ; SER_BAUD_9600
        .byte   $0F             ; SER_BAUD_19200
        .byte   $FF             ; SER_BAUD_38400
        .byte   $FF             ; SER_BAUD_57600
        .byte   $00             ; SER_BAUD_115200
        .byte   $FF             ; SER_BAUD_230400

BitTable:                       ; Table used to translate RS232 databits param
                                ; into control register value
        .byte   $60             ; SER_BITS_5
        .byte   $40             ; SER_BITS_6
        .byte   $20             ; SER_BITS_7
        .byte   $00             ; SER_BITS_8

StopTable:                      ; Table used to translate RS232 stopbits param
                                ; into control register value
        .byte   $00             ; SER_STOP_1
        .byte   $80             ; SER_STOP_2

ParityTable:                    ; Table used to translate RS232 parity param
                                ; into command register value
        .byte   $00             ; SER_PAR_NONE
        .byte   $20             ; SER_PAR_ODD
        .byte   $60             ; SER_PAR_EVEN
        .byte   $A0             ; SER_PAR_MARK
        .byte   $E0             ; SER_PAR_SPACE

IdOfsTable:                     ; Table of bytes positions, used to check four
                                ; specific bytes on the slot's firmware to make
                                ; sure this is a serial card.
        .byte   $05             ; Pascal 1.0 ID byte
        .byte   $07             ; Pascal 1.0 ID byte
        .byte   $0B             ; Pascal 1.1 generic signature byte
        .byte   $0C             ; Device signature byte

IdValTable:                     ; Table of expected values for the four checked
                                ; bytes
        .byte   $38             ; ID Byte 0 (from Pascal 1.0), fixed
        .byte   $18             ; ID Byte 1 (from Pascal 1.0), fixed
        .byte   $01             ; Generic signature for Pascal 1.1, fixed
        .byte   $31             ; Device signature byte (serial or
                                ; parallel I/O card type 1)

IdTableLen      = * - IdValTable

;----------------------------------------------------------------------------
; SER_INSTALL: Is called after the driver is loaded into memory. If possible,
; check if the hardware is present. Must return an SER_ERR_xx code in a/x.
;
; Since we don't have to manage the IRQ vector on the Apple II, this is
; actually the same as:
;
; SER_UNINSTALL: Is called before the driver is removed from memory.
; No return code required (the driver is removed from memory on return).
;
; and:
;
; SER_CLOSE: Close the port and disable interrupts. Called without parameters.
; Must return an SER_ERR_xx code in a/x.

_acia_close:
        ldx     Index           ; Check for open port
        beq     :+

        lda     #%00001010      ; Deactivate DTR and disable 6551 interrupts
        sta     ACIA_CMD,x

        ldx     #$00
        stx     Index           ; Mark port as closed
:       rts

;----------------------------------------------------------------------------
; A: slot
; X: speed
_acia_open:
        sta     Slot
        stx     Speed

        ; Check if this is a IIgs (Apple II Miscellaneous TechNote #7,
        ; Apple II Family Identification)
        sec
        bit     $C082
        jsr     $FE1F
        bit     $C080

        bcs     NotIIgs

        ; We're on a IIgs. For every slot N, either bit N of $C02D is
        ; 0 for the internal ROM, or 1 for "Your Card". Let's make sure
        ; that slot N's bit is set to 1, otherwise, that can't be an SSC.

        ldy     Slot
        lda     SLTROMSEL
:       lsr
        dey
        bpl     :-              ; Shift until slot's bit ends in carry
        bcc     NoDev

NotIIgs:ldx     #<$C000
        stx     ptr2
        lda     #>$C000
        ora     Slot
        sta     ptr2+1

:       ldy     IdOfsTable,x    ; Check Pascal 1.1 Firmware Protocol ID bytes
        lda     IdValTable,x
        cmp     (ptr2),y
        bne     NoDev
        inx
        cpx     #IdTableLen
        bcc     :-

        lda     Slot            ; Convert slot to I/O register index
        asl
        asl
        asl
        asl
        adc     #Offset         ; Assume carry to be clear
        tax

        ; Check that this works like an ACIA 6551 is expected to work

        lda     ACIA_STATUS,x   ; Save current values in what we expect to be
        sta     tmp1            ; the ACIA status register
        lda     ACIA_CMD,x      ; and command register. So we can restore them
        sta     tmp2            ; if this isn't a 6551.

        ldy     #%00000010      ; Disable TX/RX, disable IRQ
:       tya
        sta     ACIA_CMD,x
        cmp     ACIA_CMD,x      ; Verify what we stored is there
        bne     NotAcia
        iny                     ; Enable TX/RX, disable IRQ
        cpy     #%00000100
        bne     :-
        sta     ACIA_STATUS,x   ; Reset ACIA
        lda     ACIA_CMD,x      ; Check that RX/TX is disabled
        lsr
        bcc     AciaOK

NotAcia:lda     tmp2            ; Restore original values
        sta     ACIA_CMD,x
        lda     tmp1
        sta     ACIA_STATUS,x

NoDev:  lda     #SER_ERR_NO_DEVICE
        bne     Out

AciaOK: ; Set the value for the control register, which contains stop bits,
        ; word length and the baud rate.
        ldy     Speed
        lda     BaudTable,y     ; Get 6551 value
        sta     tmp2            ; Backup for IRQ setting
        bpl     BaudOK          ; Check that baudrate is supported

        lda     #SER_ERR_BAUD_UNAVAIL
        bne     Out

BaudOK: sta     tmp1
        ldy     #SER_BITS_8
        lda     BitTable,y      ; Get 6551 value
        ora     tmp1
        sta     tmp1

        ldy     #SER_STOP_1
        lda     StopTable,y     ; Get 6551 value
        ora     tmp1
        ora     #%00010000      ; Set receiver clock source = baudrate
        sta     ACIA_CTRL,x

        ; Set the value for the command register. We remember the base value
        ; in RtsOff, since we will have to manipulate ACIA_CMD often.
        ldy     #SER_PAR_NONE
        lda     ParityTable,y   ; Get 6551 value

        ora     #%00000001      ; Set DTR active
        ora     #%00001010      ; Disable interrupts and set RTS low
        sta     ACIA_CMD,x

        ; Done
        stx     Index           ; Mark port as open
        lda     #SER_ERR_OK
Out:
        ldx     #>$0000
        rts

; Push a character out of serial
_acia_put:
        pha
        ldx     Index
:       lda     ACIA_STATUS,x
        and     #$10
        beq     :-
        pla
        sta     ACIA_DATA,x
        rts

; Get character out of serial if available
; Returns with carry set if no char is available.
; Carry clear and char in A if available.
_acia_get:
        sec

        ldx     Index
        lda     ACIA_STATUS,x
        and     #$08
        beq     :+

        lda     ACIA_DATA,x
        clc
:       rts
