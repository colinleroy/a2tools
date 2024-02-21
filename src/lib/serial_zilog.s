; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .import         _baudrate, _open_slot, _flow_control
        .import         popa, pusha, popax, pushax
        .importzp       ptr3, ptr4

        .export         _simple_serial_set_speed
        .export         _simple_serial_dtr_onoff
        .export         _simple_serial_set_parity
        .export         _simple_serial_set_flow_control
        .export         _simple_serial_set_irq

        .export         _simple_serial_finish_setup
        .export         _surl_read_with_barrier

        .import         _simple_serial_read

        .include        "apple2.inc"
        .include        "ser-kernel.inc"
        .include        "ser-error.inc"

        .macpack        cpu

        .ifdef SURL_TO_LANGCARD
        .segment "LOWCODE"
        .endif

ZILOG_REG_B   := $C038
ZILOG_REG_A   := $C039
ZILOG_DATA_B  := $C03A
ZILOG_DATA_A  := $C03B

WR_TX_RX_CTRL  = 4
WR_TX_CTRL     = 5
WR_MASTER_IRQ  = 9
WR_BAUDL_CTRL  = 12
WR_BAUDH_CTRL  = 13

; FIXME does not handle 115200 bps yet. Not a problem as of now, as the
; only current user of this is the Quicktake program, which doesn't do 115200.

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

; Fixme: assumes 8 data bits, TX on, RTS on
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

; Only before opening port
_simple_serial_set_flow_control:
        sta     _flow_control
        rts

        .rodata

_simple_serial_set_irq:
        beq     :+
        lda     #%00011001
        bra     :++
:       lda     #%00000010
:       ldx     #WR_MASTER_IRQ
        stx     ZILOG_REG_B
        sta     ZILOG_REG_B
        
        rts

_simple_serial_finish_setup:
        lda     _open_slot
        beq     :+
        lda     #<ZILOG_DATA_A
        ldx     #>ZILOG_DATA_A
        sta     zilog_data_reg+1
        stx     zilog_data_reg+2
        lda     #<ZILOG_REG_A
        ldx     #>ZILOG_REG_A
        sta     zilog_status_reg+1
        stx     zilog_status_reg+2
        rts
:       lda     #<ZILOG_DATA_B
        ldx     #>ZILOG_DATA_B
        sta     zilog_data_reg+1
        stx     zilog_data_reg+2
        lda     #<ZILOG_REG_B
        ldx     #>ZILOG_REG_B
        sta     zilog_status_reg+1
        stx     zilog_status_reg+2
        rts

simple_serial_read_no_irq:
        sta     ptr3            ; Store nmemb
        stx     ptr3+1

        jsr     popax
        sta     ptr4            ; Store buffer
        stx     ptr4+1

        ldx     ptr3+1          ; Get number of full pages
        beq     last_page

        ldy     #0
        sty     check_page_done+1

do_page:
zilog_status_reg:
:       lda     $FFFF           ; Do we have a character?
        and     #$01
        beq     :-
zilog_data_reg:
        lda     $FFFF           ; We do!
        sta     (ptr4),y
        iny
check_page_done:
        cpy     #$FF            ; Patched
        bne     do_page
        inc     ptr4+1
        dex
        bmi     done
        bne     do_page
last_page:
        ldy     ptr3
        beq     done            ; Nothing to read
        sty     check_page_done+1
        ldy     #0
        beq     do_page
done:
        rts

_surl_read_with_barrier:
        pha
        phx
        lda     #0
        jsr     _simple_serial_set_irq
        lda     #$2F            ; SURL_CLIENT_READY
        jsr     _ser_put
        plx
        pla
        jsr     simple_serial_read_no_irq
        lda     #1
        jmp     _simple_serial_set_irq

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
