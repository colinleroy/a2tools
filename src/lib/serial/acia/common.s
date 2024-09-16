; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .import         _open_slot

        .export         get_acia_reg_idx
        .export         _simple_serial_set_irq
        .export         _simple_serial_get_data_reg
        .export         _simple_serial_get_status_reg

        .importzp       tmp1

        .include        "apple2.inc"
        .include        "ser-kernel.inc"
        .include        "ser-error.inc"

        .macpack        cpu
        .include        "acia.inc"

        .ifdef SURL_TO_LANGCARD
        .segment "LOWCODE"
        .endif

get_acia_reg_idx:
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

; int simple_serial_get_data_reg(char slot)
; returns data register address in AX
_simple_serial_get_data_reg:
        jsr     get_acia_reg_idx
        txa
        adc     #<ACIA
        ldx     #>ACIA
        bcc     :+
        inx
        clc
:       rts

_simple_serial_get_status_reg:
        jsr     _simple_serial_get_data_reg
        adc     #1
        rts

_simple_serial_get_cmd_reg:
        jsr     _simple_serial_get_data_reg
        adc     #2
        rts

_simple_serial_get_ctrl_reg:
        jsr     _simple_serial_get_data_reg
        adc     #3
        rts

_simple_serial_set_irq:
        tay

        lda     _open_slot
        jsr     get_acia_reg_idx
        beq     :++
        lda     ACIA_CMD,x
        and     #%11111101
        cpy     #1
        beq     :+
        ora     #%00000010
:       sta     ACIA_CMD,x
:       rts
