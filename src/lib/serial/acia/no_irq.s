; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .import         _open_slot

        .import         get_acia_reg_idx
        .export         _serial_read_byte_no_irq

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

_serial_read_byte_no_irq:
acia_status_reg:
:       lda     $FFFF           ; Do we have a character?
        and     #$08
        beq     :-
acia_data_reg:
        lda     $FFFF           ; We do!
        rts
