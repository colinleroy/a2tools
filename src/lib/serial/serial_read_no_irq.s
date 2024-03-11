; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .import         popax
        .importzp       ptr3, ptr4

        .import         _serial_read_byte_no_irq
        .export         _simple_serial_read_no_irq

        .include        "apple2.inc"
        .include        "ser-kernel.inc"
        .include        "ser-error.inc"

        .macpack        cpu

        .ifdef SURL_TO_LANGCARD
        .segment "LOWCODE"
        .endif

_simple_serial_read_no_irq:     ; Average of 17.2 cycles per byte on 8kB transfers
        php
        sei
        sta     ptr3            ; Store nmemb
        stx     ptr3+1

        jsr     popax
        sta     ptr4            ; Store buffer
        stx     ptr4+1

        ldy     #0              ; Inner loop counter

        ldx     ptr3+1          ; Get number of full pages
        beq     last_page       ; Only a partial one

:       jsr     _serial_read_byte_no_irq
        sta     (ptr4),y
        iny
        bne     :-              ; next byte

        inc     ptr4+1
        dex
        bne     :-              ; next page

last_page:
        ldx     ptr3            ; Y safely 0 there
        beq     done            ; Nothing to read

:       jsr     _serial_read_byte_no_irq
        sta     (ptr4),y
        iny
        dex
        bne     :-

done:
        plp
        rts
