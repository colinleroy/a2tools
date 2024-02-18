; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .import         _baudrate, _open_slot, _flow_control
        .import         popa, pusha, popax
        .importzp       ptr3, ptr4

        .export         _simple_serial_set_speed
        .export         _simple_serial_dtr_onoff
        .export         _simple_serial_set_parity
        .export         _simple_serial_set_flow_control

        .ifdef SERIAL_115K_HACK
        .export         _simple_serial_finish_setup
        .export         _simple_serial_read_no_irq
        .endif

        .import         _platform_msleep

        .include        "apple2.inc"
        .include        "ser-kernel.inc"
        .include        "ser-error.inc"

        .macpack        cpu

        .ifdef SURL_TO_LANGCARD
        .segment "LOWCODE"
        .endif

ZILOG_REG_B   := $C038
ZILOG_REG_A   := $C039
WR_TX_RX_CTRL  = 4
WR_TX_CTRL     = 5
WR_BAUDL_CTRL  = 12
WR_BAUDH_CTRL  = 13

_simple_serial_set_speed:
        sta     _baudrate
        tay

        lda     BaudTable,y
        ldx     #WR_BAUDL_CTRL
        stx     ZILOG_REG_B
        sta     ZILOG_REG_B

        lda     #0
        ldx     #WR_BAUDH_CTRL
        stx     ZILOG_REG_B
        sta     ZILOG_REG_B
        rts

_simple_serial_dtr_onoff:
        cmp     #0
        beq     :+

        ldx     #WR_TX_CTRL
        lda     #%01101010
        stx     ZILOG_REG_B
        sta     ZILOG_REG_B
        rts

:       ldx     #WR_TX_CTRL
        lda     #%11101010
        stx     ZILOG_REG_B
        sta     ZILOG_REG_B
        rts

; Fixme: assumes 8 data bits/1 stop bit
_simple_serial_set_parity:
        tay

        ldx     #WR_TX_RX_CTRL
        lda     ParityTable,y
        stx     ZILOG_REG_B
        sta     ZILOG_REG_B
        rts

_simple_serial_set_flow_control:
        sta     _flow_control
        rts

        .rodata

BaudTable:                      ; Table used to translate RS232 baudrate param
                                ; into control register value
                                ; bit7 = 1 means setting is invalid
        .byte   $FF             ; SER_BAUD_45_5
        .byte   $FF             ; SER_BAUD_50
        .byte   $FF             ; SER_BAUD_75
        .byte   $FF             ; SER_BAUD_110
        .byte   $FF             ; SER_BAUD_134_5
        .byte   $FF             ; SER_BAUD_150
        .byte   $7E             ; SER_BAUD_300
        .byte   $FF             ; SER_BAUD_600
        .byte   $5E             ; SER_BAUD_1200
        .byte   $FF             ; SER_BAUD_1800
        .byte   $2E             ; SER_BAUD_2400
        .byte   $FF             ; SER_BAUD_3600
        .byte   $16             ; SER_BAUD_4800
        .byte   $FF             ; SER_BAUD_7200
        .byte   $0A             ; SER_BAUD_9600
        .byte   $04             ; SER_BAUD_19200
        .byte   $01             ; SER_BAUD_38400
        .byte   $00             ; SER_BAUD_57600
        .byte   $FF             ; SER_BAUD_115200
        .byte   $FF             ; SER_BAUD_230400

ParityTable:                    ; Table used to translate RS232 parity param
                                ; into command register value
        .byte   %01000100       ; SER_PAR_NONE
        .byte   %01000101       ; SER_PAR_ODD
        .byte   %01000111       ; SER_PAR_EVEN
        .byte   $FF             ; SER_PAR_MARK
        .byte   $FF             ; SER_PAR_SPACE
