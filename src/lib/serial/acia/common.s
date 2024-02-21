; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .import         _open_slot

        .export         get_acia_reg_idx
        .export         _simple_serial_set_irq

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
