; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .import         _open_slot
        .import         _simple_serial_read

        .export         _serial_read_byte_no_irq
        .export         _simple_serial_setup_no_irq_regs


        .include        "apple2.inc"
        .include        "ser-kernel.inc"
        .include        "ser-error.inc"

        .macpack        cpu
        .include        "z8530.inc"

        .ifdef SURL_TO_LANGCARD
        .segment "LOWCODE"
        .endif

_simple_serial_setup_no_irq_regs:
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

_serial_read_byte_no_irq:
zilog_status_reg:
:       lda     $FFFF           ; Do we have a character?
        and     #$01
        beq     :-
zilog_data_reg:
        lda     $FFFF           ; We do!
        rts