; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .import         _open_slot

        .export         _simple_serial_set_irq
        .export         _simple_serial_get_data_reg
        .export         _simple_serial_get_status_reg

        .include        "apple2.inc"
        .include        "ser-kernel.inc"
        .include        "ser-error.inc"

        .macpack        cpu
        .include        "z8530.inc"

        .ifdef SURL_TO_LANGCARD
        .segment "LOWCODE"
        .endif

_simple_serial_set_irq:
        beq     :+
        lda     #%00011001
        bra     :++
:       lda     #%00000010
:       ldx     #WR_MASTER_IRQ
        stx     ZILOG_REG_B
        sta     ZILOG_REG_B
        
        rts

; int simple_serial_get_data_reg(char slot)
; returns data register address in AX
_simple_serial_get_data_reg:
        cmp     #0
        beq     :+
        lda     #<ZILOG_DATA_A
        ldx     #>ZILOG_DATA_A
        rts
:       lda     #<ZILOG_DATA_B
        ldx     #>ZILOG_DATA_B
        rts

; int simple_serial_get_status_reg(char slot)
; returns status register address in AX
_simple_serial_get_status_reg:
        cmp     #0
        beq     :+
        lda     #<ZILOG_REG_A
        ldx     #>ZILOG_REG_A
        rts
:       lda     #<ZILOG_REG_B
        ldx     #>ZILOG_REG_B
        rts
