; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .import         _open_slot

        .import         get_acia_reg_idx
        .export         _serial_read_byte_no_irq
        .export         _serial_putc_direct
        .export         _simple_serial_setup_no_irq_regs

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
        jsr     get_acia_reg_idx
        txa
        clc
        adc     #<ACIA_STATUS
        sta     acia_status_reg_r+1
        sta     acia_status_reg_w+1
        lda     #>ACIA_STATUS
        adc     #0
        sta     acia_status_reg_r+2
        sta     acia_status_reg_w+2
        txa
        clc
        adc     #<ACIA_DATA
        sta     acia_data_reg_r+1
        sta     acia_data_reg_w+1
        lda     #>ACIA_DATA
        adc     #0
        sta     acia_data_reg_r+2
        sta     acia_data_reg_w+2
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
