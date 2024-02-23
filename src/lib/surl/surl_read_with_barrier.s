; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .import         _serial_putc_direct
        .import         _simple_serial_set_irq
        .import         _simple_serial_read_no_irq

        .export         _surl_read_with_barrier

        .include        "apple2.inc"
        .include        "ser-kernel.inc"
        .include        "ser-error.inc"

        .macpack        cpu

        .ifdef SURL_TO_LANGCARD
        .segment "LOWCODE"
        .endif

_surl_read_with_barrier:
        pha
        .if (.cpu .bitand CPU_ISET_65C02)
        phx
        .else
        txa
        pha
        .endif
        lda     #0
        jsr     _simple_serial_set_irq
        lda     #$2F            ; SURL_CLIENT_READY
        jsr     _serial_putc_direct
        .if (.cpu .bitand CPU_ISET_65C02)
        plx
        .else
        pla
        tax
        .endif
        pla
        jsr     _simple_serial_read_no_irq
        lda     #1
        jmp     _simple_serial_set_irq
