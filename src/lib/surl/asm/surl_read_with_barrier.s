; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .import         _serial_putc_direct
        .import         _simple_serial_set_irq
        .import         throbber_on, throbber_off

        .import         popax
        .importzp       ptr3, ptr4
        .export         _surl_read_with_barrier
        .import         _serial_read_byte_no_irq

        .include        "apple2.inc"
        .include        "ser-kernel.inc"
        .include        "ser-error.inc"

        .macpack        cpu

        .ifdef SURL_TO_LANGCARD
        .segment "LOWCODE"
        .endif

.proc _surl_read_with_barrier: near
        php
        sei
        sta     ptr3            ; Store nmemb
        stx     ptr3+1

        jsr     popax
        sta     ptr4            ; Store buffer
        stx     ptr4+1

        jsr     throbber_on

        ldy     #0              ; Inner loop counter

        lda     #0
        jsr     _simple_serial_set_irq

        lda     #$2F            ; SURL_CLIENT_READY
        jsr     _serial_putc_direct

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
        lda     #1
        jsr     _simple_serial_set_irq

        plp
        jmp     throbber_off
.endproc
