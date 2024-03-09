; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .import         _serial_putc_direct
        .import         _simple_serial_set_irq
        .import         _simple_serial_read_no_irq

        .importzp       tmp1, tmp2
        .export         _surl_read_with_barrier

        .include        "apple2.inc"
        .include        "ser-kernel.inc"
        .include        "ser-error.inc"

        .macpack        cpu

        .ifdef SURL_TO_LANGCARD
        .segment "LOWCODE"
        .endif

_surl_read_with_barrier:
        sta     tmp1
        stx     tmp2

        lda     #0
        jsr     _simple_serial_set_irq

        ldx     tmp2

        lda     #$2F            ; SURL_CLIENT_READY
        jsr     _serial_putc_direct

        lda     tmp1
        jsr     _simple_serial_read_no_irq
        lda     #1
        jmp     _simple_serial_set_irq
