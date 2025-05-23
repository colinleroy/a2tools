; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .import         _baudrate, _open_slot, _flow_control
        .import         popa, pusha

        .import         get_acia_reg_idx

        .export         _simple_serial_set_speed
        .export         _simple_serial_slot_dtr_onoff
        .export         _simple_serial_dtr_onoff
        .export         _simple_serial_set_parity
        .export         _simple_serial_set_flow_control

        .include        "apple2.inc"
        .include        "ser-kernel.inc"
        .include        "ser-error.inc"

        .macpack        cpu
        .include        "acia.inc"

        .ifdef SURL_TO_LANGCARD
        .segment        "LOWCODE"
        .endif

_simple_serial_set_speed:
        sta     _baudrate
        tay

        lda     _open_slot
        jsr     get_acia_reg_idx
        beq     set_speed_done

        lda     ACIA_CTRL,x
        and     #%11110000
        ora     BaudTable,y
        sta     ACIA_CTRL,x
set_speed_done:
        rts

_simple_serial_slot_dtr_onoff:
        tay
        jsr     popa
        jsr     get_acia_reg_idx
        beq     :++
        lda     ACIA_CMD,x
        and     #%11110000
        cpy     #0
        beq     :+
        ora     #%00001011
:       sta     ACIA_CMD,x
:       rts

_simple_serial_dtr_onoff:
        tay
        lda     _open_slot
        jsr     pusha
        tya
        jmp     _simple_serial_slot_dtr_onoff

_simple_serial_set_parity:
        tay

        lda     _open_slot
        jsr     get_acia_reg_idx
        beq     :+
        lda     ACIA_CMD,x
        and     #%00011111
        ora     ParityTable,y
        sta     ACIA_CMD,x
:       rts

_simple_serial_set_flow_control:
        sta     _flow_control
        rts

        .rodata

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

ParityTable:                    ; Table used to translate RS232 parity param
                                ; into command register value
        .byte   $00             ; SER_PAR_NONE
        .byte   $20             ; SER_PAR_ODD
        .byte   $60             ; SER_PAR_EVEN
        .byte   $A0             ; SER_PAR_MARK
        .byte   $E0             ; SER_PAR_SPACE
