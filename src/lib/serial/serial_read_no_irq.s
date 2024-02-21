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

_simple_serial_read_no_irq:
        sta     ptr3            ; Store nmemb
        stx     ptr3+1

        jsr     popax
        sta     ptr4            ; Store buffer
        stx     ptr4+1

        ldx     ptr3+1          ; Get number of full pages
        beq     last_page

        ldy     #0
        sty     check_page_done+1

do_page:
        jsr     _serial_read_byte_no_irq
        sta     (ptr4),y

        iny
check_page_done:
        cpy     #$FF            ; Patched
        bne     do_page
        inc     ptr4+1
        dex
        bmi     done
        bne     do_page
last_page:
        ldy     ptr3
        beq     done            ; Nothing to read
        sty     check_page_done+1
        ldy     #0
        beq     do_page
done:
        rts
