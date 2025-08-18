;
; Serial driver for the Apple IIgs Zilog Z8530.
;
; Colin Leroy-Mira <colin@colino.net>, 2023
;
; This software is licensed under the same license as cc65,
; the zlib license (see LICENSE file).
;
; Documentation from http://www.applelogic.org/files/Z8530UM.pdf (pages
; referred to where applicable)
; and https://gswv.apple2.org.za/a2zine/Utils/Z8530_SCCsamples_info.txt

        .export         _z8530_open, _z8530_close
        .export         _z8530_put, _z8530_read_byte_sync

.ifdef SERIAL_ENABLE_IRQ
        .export         _z8530_irq, _z8530_set_irq
.endif

.ifdef SERIAL_LOW_LEVEL_CONTROL
        .export         _z8530_set_speed
        .export         _z8530_slot_dtr_onoff
        .export         _z8530_set_parity
.endif

        .import         _ser_status_reg, _ser_data_reg

        .import         popa, _baudrate

        .setcpu         "65816"

        .include        "zeropage.inc"
        .include        "ser-kernel.inc"
        .include        "ser-error.inc"

CurClockSource: .res    1               ; Whether to use BRG or RTxC for clock
Slot:           .res    1
Speed:          .res    1

Opened:         .byte   $00             ; 1 when opened
Channel:        .byte   $00             ; Channel B by default
CurChanIrqFlags:.byte   $00

SerFlagOrig:    .byte   $00

RxBitTable:     .byte   %00000000       ; SER_BITS_5, in WR_RX_CTRL (WR3)
                .byte   %10000000       ; SER_BITS_6  (Ref page 5-7)
                .byte   %01000000       ; SER_BITS_7
                .byte   %11000000       ; SER_BITS_8

TxBitTable:     .byte   %00000000       ; SER_BITS_5, in WR_TX_CTRL (WR5)
                .byte   %01000000       ; SER_BITS_6  (Ref page 5-9)
                .byte   %00100000       ; SER_BITS_7
                .byte   %01100000       ; SER_BITS_8

ClockMultiplier:.byte   %01000000       ; Clock x16 (300-57600bps, WR4, ref page 5-8)
                .byte   %10000000       ; Clock x32 (115200bps, ref page 5-8)

ClockSource:    .byte   %01010000       ; Use baud rate generator (ch. B) (WR11, page 5-17)
                .byte   %00000000       ; Use RTxC (115200bps) (ch. B)
                .byte   %11010000       ; Use baud rate generator (ch. A)
                .byte   %10000000       ; Use RTxC (115200bps) (ch. A)

BrgEnabled:     .byte   %00000001       ; Baud rate generator on (WR14, page 5-19)
                .byte   %00000000       ; BRG Off

ChanIrqFlags:   .byte %00000101         ; ANDed (RX/special IRQ, ch. B) (page 5-25)
                .byte %00101000         ; ANDed (RX/special IRQ, ch. A)

ChanIrqMask:    .byte %00000111         ; Ch. B IRQ flags mask
                .byte %00111000         ; Ch. A IRQ flags mask

BaudTable:                              ; bit7 = 1 means setting is invalid
                                        ; Indexes cc65 RS232 SER_BAUD enum
                                        ; into WR12/13 register values
                                        ; (Ref page 5-18 and 5-19)
                .word   $FFFF           ; SER_BAUD_45_5
                .word   $FFFF           ; SER_BAUD_50
                .word   $FFFF           ; SER_BAUD_75
                .word   $FFFF           ; SER_BAUD_110
                .word   $FFFF           ; SER_BAUD_134_5
                .word   $FFFF           ; SER_BAUD_150
                .word   $017E           ; SER_BAUD_300
                .word   $FFFF           ; SER_BAUD_600
                .word   $005E           ; SER_BAUD_1200
                .word   $FFFF           ; SER_BAUD_1800
                .word   $002E           ; SER_BAUD_2400
                .word   $FFFF           ; SER_BAUD_3600
                .word   $0016           ; SER_BAUD_4800
                .word   $FFFF           ; SER_BAUD_7200
                .word   $000A           ; SER_BAUD_9600
                .word   $0004           ; SER_BAUD_19200
                .word   $0001           ; SER_BAUD_38400
                .word   $0000           ; SER_BAUD_57600
                .word   $0000           ; SER_BAUD_115200 (constant unused at that speed)
                .word   $FFFF           ; SER_BAUD_230400

; About the speed selection: either we use the baud rate generator:
; - Load the time constants from BaudTable into WR12/WR13
; - Setup the TX/RX clock source to BRG (ClockSource into WR11)
; - Setup the clock multiplier (WR4)
; - Enable the baud rate generator (WR14)
; In this case, the baud rate will be:
;    rate = crystal_clock/(2+BRG_time_constant))/(2*clock_multiplier)
; Example: (3686400/(2+0x0004)) / (2*16) = 19200 bps
;
; Or we don't use the baud rate generator:
; - Setup the TX/RX clock source to RTxC
; - Setup the clock multiplier
; - Disable the baud rate generator
; - WR12 and 13 are ignored
; In this case, the baud rate will be:
;    rate = crystal_clock/clock_multiplier
; Example: 3686400/32 = 115200 bps

StopTable:      .byte   %00000100       ; SER_STOP_1, in WR_TX_RX_CTRL (WR4)
                .byte   %00001100       ; SER_STOP_2  (Ref page 5-8)

ParityTable:    .byte   %00000000       ; SER_PAR_NONE, in WR_TX_RX_CTRL (WR4)
                .byte   %00000001       ; SER_PAR_ODD   (Ref page 5-8)
                .byte   %00000011       ; SER_PAR_EVEN
                .byte   $FF             ; SER_PAR_MARK
                .byte   $FF             ; SER_PAR_SPACE

; ------------------------------------------------------------------------
; Addresses

SCCAREG    := $C039
SCCBREG    := $C038
SCCADATA   := $C03B
SCCBDATA   := $C03A

; We're supposed to get SerFlag's address using GetAddr on ROMs 1 and 3.
; (https://archive.org/details/IIgs_2523018_SCC_Access, page 9)
; But, it's the same value as on ROM0. As we don't expect a ROM 4 anytime
; soon with a different value, let's keep it simple.

SER_FLAG   := $E10104

; ------------------------------------------------------------------------
; Channels

CHANNEL_B              = 0
CHANNEL_A              = 1

; ------------------------------------------------------------------------
; Write registers, read registers, and values that interest us

WR_INIT_CTRL           = 0
RR_INIT_STATUS         = 0
INIT_CTRL_CLEAR_EIRQ   = %00010000
INIT_CTRL_CLEAR_ERR    = %00110000
INIT_STATUS_READY      = %00000100
INIT_STATUS_RTS        = %00100000

WR_TX_RX_MODE_CTRL     = 1
TX_RX_MODE_OFF         = %00000000
TX_RX_MODE_RXIRQ       = %00010001

WR_RX_CTRL             = 3              ; (Ref page 5-7)
RR_RX_STATUS           = 9              ; Corresponding status register
RX_CTRL_ON             = %00000001      ; ORed, Rx enabled
RX_CTRL_OFF            = %11111110      ; ANDed,Rx disabled

WR_TX_RX_CTRL          = 4
RR_TX_RX_STATUS        = 4

WR_TX_CTRL             = 5              ; (Ref page 5-9)
RR_TX_STATUS           = 5              ; Corresponding status register
TX_CTRL_ON             = %00001000      ; ORed, Tx enabled
TX_CTRL_OFF            = %11110111      ; ANDed,Tx disabled
TX_DTR_ON              = %01111111      ; ANDed,DTR ON (high)
TX_DTR_OFF             = %10000000      ; ORed, DTR OFF
TX_RTS_ON              = %00000010      ; ORed, RTS ON (low)
TX_RTS_OFF             = %11111101      ; ANDed, RTS OFF

WR_MASTER_IRQ_RST      = 9              ; (Ref page 5-14)
MASTER_IRQ_SHUTDOWN    = %00000010      ; STA'd
MASTER_IRQ_MIE_RST     = %00001010      ; STA'd
MASTER_IRQ_SET         = %00011001      ; STA'd

WR_CLOCK_CTRL          = 11             ; (Ref page 5-17)

WR_BAUDL_CTRL          = 12             ; (Ref page 5-18)
WR_BAUDH_CTRL          = 13             ; (Ref page 5-19)

WR_MISC_CTRL           = 14             ; (Ref page 5-19)

WR_IRQ_CTRL            = 15             ; (Ref page 5-20)
IRQ_CLEANUP_EIRQ       = %00001000

RR_SPEC_COND_STATUS    = 1              ; (Ref page 5-23)
SPEC_COND_FRAMING_ERR  = %01000000
SPEC_COND_OVERRUN_ERR  = %00100000

RR_IRQ_STATUS          = 2              ; (Ref page 5-24)
IRQ_MASQ               = %01110000      ; ANDed
IRQ_RX                 = %00100000
IRQ_SPECIAL            = %01100000

RR_INTR_PENDING_STATUS = 3              ; (Ref page 5-25)
INTR_IS_RX             = %00100100      ; ANDed (RX IRQ, channel A or B)

; Read register value to A.
; Input:  X as channel
;         Y as register
; Output: A
readSSCReg:
      cpx       #0
      bne       ReadAreg
      sty       SCCBREG
      lda       SCCBREG
      rts
ReadAreg:
      sty       SCCAREG
      lda       SCCAREG
      rts

; Write value of A to a register.
; Input: X as channel
;        Y as register
writeSCCReg:
      cpx       #0
      bne       WriteAreg
      sty       SCCBREG
      sta       SCCBREG
      rts
WriteAreg:
      sty       SCCAREG
      sta       SCCAREG
      rts

_z8530_close:
        ldx     Opened                  ; Check for open port
        beq     :+

        ldx     Channel

        php                             ; Deactivate interrupts
        sei                             ; if enabled

        ldy     #WR_MASTER_IRQ_RST
        lda     #MASTER_IRQ_SHUTDOWN
        jsr     writeSCCReg

        ldy     #WR_TX_RX_MODE_CTRL
        lda     #TX_RX_MODE_OFF
        jsr     writeSCCReg

        ; Reset SerFlag to what it was
        lda     SerFlagOrig
        sta     SER_FLAG

        lda     SCCBDATA

        ; Clear external interrupts (twice)
        ldy     #WR_INIT_CTRL
        lda     #INIT_CTRL_CLEAR_EIRQ
        jsr     writeSCCReg
        jsr     writeSCCReg

        ; Reset MIE for firmware use
        ldy     #WR_MASTER_IRQ_RST
        lda     #MASTER_IRQ_MIE_RST
        jsr     writeSCCReg

        ldx     #$00
        stx     Opened                  ; Mark port as closed

        plp                             ; Reenable interrupts if needed
:       txa                             ; Promote char return value
        rts

getClockSource:
        .assert SER_PARAMS::BAUDRATE = 0, error
        lda     (ptr1)                  ; Baudrate index - cc65 value
        cmp     #SER_BAUD_115200
        lda     #$00
        adc     #$00
        sta     CurClockSource          ; 0 = BRG, 1 = RTxC
        rts

;----------------------------------------------------------------------------
; A: slot (channel)
; X: speed
_z8530_open:
        cmp     #2                      ; Check port validity (0 or 1)
        bcc     :+

        lda     #$FF
        tax
        rts

:       php                             ; Deactivate interrupts
        sei                             ; if enabled

        sta     Channel
        stx     Speed

        tax

        ldy     #RR_INIT_STATUS         ; Hit rr0 once to sync up
        jsr     readSSCReg

        ldy     #WR_MISC_CTRL           ; WR14: Turn everything off
        lda     #$00
        jsr     writeSCCReg

        jsr     getClockSource          ; Should we use BRG or RTxC?

        ldy     #SER_STOP_1             ; WR4 setup: clock mult., stop & parity
        lda     StopTable,y             ; Get value

        ldy     #SER_PAR_NONE
        ora     ParityTable,y           ; Get value
        bmi     InvParam

        ldy     CurClockSource          ; Clock multiplier
        ora     ClockMultiplier,y

        ldy     #WR_TX_RX_CTRL
        jsr     writeSCCReg             ; End of WR4 setup

        ldy     CurClockSource          ; WR11 setup: clock source
        cpx     #CHANNEL_B
        beq     SetClock
        iny                             ; Shift to get correct ClockSource val
        iny                             ; depending on our channel

SetClock:
        lda     ClockSource,y
        ldy     #WR_CLOCK_CTRL
        jsr     writeSCCReg             ; End of WR11 setup

        lda     ChanIrqFlags,x          ; Store which IRQ bits we'll check
        sta     CurChanIrqFlags

SetBaud:
        lda     Speed
        asl
        tay

        lda     BaudTable,y             ; Get low byte of register value
        bpl     BaudOK                  ; Verify baudrate is supported

InvParam:
        lda     #SER_ERR_INIT_FAILED
        ldy     #$00                    ; Mark port closed
        jmp     SetupOut

BaudOK:
        phy                             ; WR12 setup: BRG time constant, low byte
        ldy     #WR_BAUDL_CTRL          ; Setting WR12 & 13 is useless if we're using
        jsr     writeSCCReg             ; RTxC, but doing it anyway makes code smaller
        ply

        iny
        lda     BaudTable,y             ; WR13 setup: BRG time constant, high byte
        ldy     #WR_BAUDH_CTRL
        jsr     writeSCCReg

        ldy     CurClockSource          ; WR14 setup: BRG enabling
        lda     BrgEnabled,y
        ldy     #WR_MISC_CTRL           ; Time to turn this thing on
        jsr     writeSCCReg

        ldy     #SER_BITS_8             ; WR3 setup: RX data bits
        lda     RxBitTable,y
        ora     #RX_CTRL_ON             ; and turn receiver on

        phy
        ldy     #WR_RX_CTRL
        jsr     writeSCCReg             ; End of WR3 setup
        ply

        lda     TxBitTable,y            ; WR5 setup: TX data bits
        ora     #TX_CTRL_ON             ; and turn transmitter on
        and     #TX_DTR_ON              ; and turn DTR on
        ora     #TX_RTS_ON              ; and turn RTS on

        ldy     #WR_TX_CTRL
        jsr     writeSCCReg             ; End of WR5 setup

        ldy     #WR_IRQ_CTRL            ; WR15 setup: IRQ
        lda     #IRQ_CLEANUP_EIRQ
        jsr     writeSCCReg

        ldy     #WR_INIT_CTRL           ; WR0 setup: clear existing IRQs
        lda     #INIT_CTRL_CLEAR_EIRQ
        jsr     writeSCCReg             ; Clear (write twice)
        jsr     writeSCCReg

        ldy     #WR_TX_RX_MODE_CTRL     ; WR1 setup: Activate RX IRQ
        lda     #TX_RX_MODE_RXIRQ
        jsr     writeSCCReg
        
        lda     SCCBREG                 ; WR9 setup: Deactivate master IRQ
        ldy     #WR_MASTER_IRQ_RST
        lda     #MASTER_IRQ_SHUTDOWN
        jsr     writeSCCReg

        lda     SER_FLAG                ; Get SerFlag's current value
        sta     SerFlagOrig             ; and save it
        
        ora     ChanIrqMask,x           ; Tell firmware which channel IRQs we want
        sta     SER_FLAG

        clc
        lda     Channel
        adc     #<$C038
        sta     status_reg_w_1+1
        sta     status_reg_w_2+1
        sta     z8530_read_status_reg+1
        sta     _ser_status_reg

        lda     Channel
        adc     #<$C03A
        sta     data_reg_w+1
        sta     z8530_read_data_reg+1
        sta     _ser_data_reg

        lda     #>$C000
        sta     _ser_status_reg+1
        sta     _ser_data_reg+1

        ldy     #$01                    ; Mark port opened
        lda     #SER_ERR_OK

SetupOut:
        plp                             ; Reenable interrupts if needed
        ldx     #>$0000
        sty     Opened
        rts

; Push a character out of serial
_z8530_put:
        pha
:       lda     #$00            ;
status_reg_w_1:
        sta     $C038           ; Patched (status reg)
status_reg_w_2:
        lda     $C038
        and     #%00000100      ; Init status ready?
        beq     :-

        pla
data_reg_w:
        sta     $C03A
        rts

; Get character out of serial if available
; Returns with carry set if no char is available.
; Carry clear and char in A if available.
_z8530_read_byte_sync:
        sec
z8530_read_status_reg:
        lda     $C038
        and     #$01
        beq     :+

z8530_read_data_reg:
        lda     $C03A
        clc
:       rts

.ifdef SERIAL_ENABLE_IRQ
_z8530_irq:
        ldy     #RR_INTR_PENDING_STATUS ; IRQ status is always in A reg
        sty     SCCAREG
        lda     SCCAREG

        and     CurChanIrqFlags         ; Is this ours?
        beq     Done

        and     #INTR_IS_RX             ; Is this an RX irq?
        beq     CheckSpecial

        ldx     Channel
        beq     ReadBdata
        lda     SCCADATA
        bra     ReadDone
ReadBdata:
        lda     SCCBDATA                ; Get byte
ReadDone:
        jmp     IRQDone

CheckSpecial:
        ; Always check IRQ special flags from Channel B (Ref page 5-24)
        ldy     #RR_IRQ_STATUS
        sty     SCCBREG
        lda     SCCBREG

        and     #IRQ_MASQ
        cmp     #IRQ_SPECIAL
        beq     Special

        ; Clear exint
        ldx     Channel
        ldy     #WR_INIT_CTRL
        lda     #INIT_CTRL_CLEAR_EIRQ
        jsr     writeSCCReg
IRQDone:sec                             ; Interrupt handled
Done:   rts

Special:ldx     Channel
        ldy     #RR_SPEC_COND_STATUS
        jsr     readSSCReg

        tax
        and     #SPEC_COND_FRAMING_ERR
        bne     BadChar
        txa
        and     #SPEC_COND_OVERRUN_ERR
        beq     BadChar

        ldy     #WR_INIT_CTRL
        lda     #INIT_CTRL_CLEAR_ERR
        jsr     writeSCCReg

        sec
        rts

BadChar:
        cpx     #CHANNEL_B
        beq     BadCharB
        lda     SCCADATA
        bra     BadCharDone
BadCharB:
        lda     SCCBDATA                ; Remove char in error
BadCharDone:
        sec
        rts

_z8530_set_irq:
        beq     :+
        lda     #MASTER_IRQ_SET
        bra     :++
:       lda     #MASTER_IRQ_SHUTDOWN
:       ldx     #WR_MASTER_IRQ_RST
        stx     SCCBREG
        sta     SCCBREG
        rts
.endif ; .ifdef SERIAL_ENABLE_IRQ

.ifdef SERIAL_LOW_LEVEL_CONTROL
; FIXME does not handle 115200 bps yet. Not a problem as of now, as the
; only current user of this function is the Quicktake program, which
; doesn't do 115200. cc65's driver can open at SER_BAUD_115200 and that's
; sufficient for the surl-based programs.

_z8530_set_speed:
        sta     _baudrate
        tay

        lda     BaudTable,y
        ldx     #WR_BAUDL_CTRL
        stx     SCCBREG
        sta     SCCBREG

        lda     #0
        ldx     #WR_BAUDH_CTRL
        stx     SCCBREG
        sta     SCCBREG
        rts

; Fixme: assumes 8 data bits, TX on, RTS on. Same thing as previously:
; this is sufficient for surl-based uses. Slot ignored.

_z8530_slot_dtr_onoff:
        sty     tmp1
        jsr     popa
        ldy     tmp1
        cpy     #0
        beq     :+

        ldx     #WR_TX_CTRL
        lda     #%01101010
        stx     SCCBREG
        sta     SCCBREG
        rts

:       ldx     #WR_TX_CTRL
        lda     #%11101010
        stx     SCCBREG
        sta     SCCBREG
        rts

; Fixme: assumes 8 data bits/1 stop bit. Same thing as previously:
; this is sufficient for surl-based uses.

_z8530_set_parity:
        tay

        ldx     #WR_TX_RX_CTRL
        lda     ParityTable,y
        stx     SCCBREG
        sta     SCCBREG
        rts
.endif ;.ifdef SERIAL_LOW_LEVEL_CONTROL
