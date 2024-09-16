; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .import         _open_slot
        .import         _simple_serial_get_data_reg
        .import         _simple_serial_get_status_reg

        .export         _serial_read_byte_no_irq
        .export         _serial_putc_direct
        .export         _simple_serial_setup_no_irq_regs

        .export         acia_status_reg_r, acia_data_reg_r
        .include        "apple2.inc"
        .include        "ser-kernel.inc"
        .include        "ser-error.inc"

        .macpack        cpu
        .include        "acia.inc"

        .ifdef SURL_TO_LANGCARD
        .segment "LOWCODE"
        .endif

_simple_serial_setup_no_irq_regs:
        lda     _open_slot
        jsr     _simple_serial_get_status_reg
        sta     acia_status_reg_r+1
        sta     acia_status_reg_w+1
        stx     acia_status_reg_r+2
        stx     acia_status_reg_w+2

        lda     _open_slot
        jsr     _simple_serial_get_data_reg
        sta     acia_data_reg_r+1
        sta     acia_data_reg_w+1
        stx     acia_data_reg_r+2
        stx     acia_data_reg_w+2
        rts

_serial_read_byte_no_irq:
acia_status_reg_r:
:       lda     $FFFF           ; Do we have a character?
        and     #$08
        beq     :-
acia_data_reg_r:
        lda     $FFFF           ; We do!
        rts

_serial_putc_direct:
        pha
acia_status_reg_w:
:       lda     $FFFF
        and     #$10
        beq     :-
        pla
acia_data_reg_w:
        sta     $FFFF
        rts
