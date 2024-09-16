; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .importzp       tmp2
        .import         _open_slot
        .import         _simple_serial_read
        .import         _simple_serial_get_data_reg
        .import         _simple_serial_get_status_reg

        .export         _serial_putc_direct
        .export         _serial_read_byte_no_irq
        .export         _simple_serial_setup_no_irq_regs

        .export         zilog_status_reg_r, zilog_data_reg_r
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
        jsr     _simple_serial_get_data_reg
        sta     zilog_data_reg_r+1
        stx     zilog_data_reg_r+2
        sta     zilog_data_reg_w+1
        stx     zilog_data_reg_w+2
        lda     _open_slot
        jsr     _simple_serial_get_status_reg
        sta     zilog_status_reg_r+1
        stx     zilog_status_reg_r+2
        sta     zilog_status_reg_w+1
        stx     zilog_status_reg_w+2
        sta     zilog_status_reg_w2+1
        stx     zilog_status_reg_w2+2
        rts

_serial_read_byte_no_irq:
zilog_status_reg_r:
:       lda     $FFFF           ; Do we have a character?
        and     #$01
        beq     :-
zilog_data_reg_r:
        lda     $FFFF           ; We do!
        rts

_serial_putc_direct:
        pha
:       lda     #$00            ;
zilog_status_reg_w:
        sta     $FFFF           ; Patched (status reg)
zilog_status_reg_w2:
        lda     $FFFF
        and     #%00000100      ; Init status ready?
        beq     :-

        pla
zilog_data_reg_w:
        sta     $FFFF
        rts
