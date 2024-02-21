; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .import         _baudrate, _open_slot, _flow_control
        .import         popa, pusha, popax
        .importzp       ptr3, ptr4

        .export         _simple_serial_set_speed
        .export         _simple_serial_acia_onoff
        .export         _simple_serial_dtr_onoff
        .export         _simple_serial_set_parity
        .export         _simple_serial_set_flow_control

        .export         _simple_serial_finish_setup
        .export         _simple_serial_set_irq
        .export         _surl_read_with_barrier

        .include        "apple2.inc"
        .include        "ser-kernel.inc"
        .include        "ser-error.inc"

        .macpack        cpu

        .ifdef SURL_TO_LANGCARD
        .segment "LOWCODE"
        .endif

.if (.cpu .bitand CPU_ISET_65C02)
ACIA           := $C088
.else
Offset          = $8F           ; Move 6502 false read out of I/O to page $BF
ACIA           := $C088-Offset
.endif

ACIA_DATA      := ACIA+0        ; Data register
ACIA_STATUS    := ACIA+1        ; Status register
ACIA_CMD       := ACIA+2        ; Command register
ACIA_CTRL      := ACIA+3        ; Control register

get_reg_idx:
        cmp     #0
        beq     not_open
        asl
        asl
        asl
        asl
        .if .not (.cpu .bitand CPU_ISET_65C02)
        clc
        adc     #Offset
        .endif
        tax
        rts
not_open:
        ldx     #00
        rts

_simple_serial_set_speed:
        sta     _baudrate
        tay

        lda     _open_slot
        jsr     get_reg_idx
        beq     set_speed_done

        lda     ACIA_CTRL,x
        and     #%11110000
        ora     BaudTable,y
        sta     ACIA_CTRL,x
set_speed_done:
        rts

_simple_serial_acia_onoff:
        tay
        jsr     popa
        jsr     get_reg_idx
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
        jmp     _simple_serial_acia_onoff

_simple_serial_set_parity:
        tay

        lda     _open_slot
        jsr     get_reg_idx
        beq     :+
        lda     ACIA_CMD,x
        and     #%00011111
        ora     ParityTable,y
        sta     ACIA_CMD,x
:       rts

_simple_serial_set_flow_control:
        sta     _flow_control
        rts

_simple_serial_set_irq:
        tay

        lda     _open_slot
        jsr     get_reg_idx
        beq     :++
        lda     ACIA_CMD,x
        and     #%11111101
        cpy     #1
        beq     :+
        ora     #%00000010
:       sta     ACIA_CMD,x
:       rts

_simple_serial_finish_setup:
        lda     _open_slot
        jsr     get_reg_idx
        txa
        clc
        adc     #<ACIA_STATUS
        sta     acia_status_reg+1
        lda     #>ACIA_STATUS
        adc     #0
        sta     acia_status_reg+2
        txa
        clc
        adc     #<ACIA_DATA
        sta     acia_data_reg+1
        lda     #>ACIA_DATA
        adc     #0
        sta     acia_data_reg+2
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
acia_status_reg:
:       lda     $FFFF           ; Do we have a character?
        and     #$08
        beq     :-
acia_data_reg:
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
        txa
        pha
        lda     #0
        jsr     _simple_serial_set_irq

        lda     #$2F            ; SURL_CLIENT_READY
        jsr     _ser_put
        pla
        tax
        pla
        jsr     simple_serial_read_no_irq
        lda     #1
        jmp     _simple_serial_set_irq

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
