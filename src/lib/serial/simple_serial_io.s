;
; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .export         _simple_serial_puts
        .export         _simple_serial_getc
        .export         _simple_serial_write
        .export         _simple_serial_flush
        .export         simple_serial_compute_ptr_end

        .ifdef          SIMPLE_SERIAL_DUMP
        .export         _simple_serial_dump
        .import         popa
        .endif

        .import         _simple_serial_getc_with_timeout
        .import         _ser_get, _serial_putc_direct, _strlen
        .import         pushax, popax
        .importzp       tmp2, ptr3, ptr4
        .include        "apple2.inc"
        .include        "ser-error.inc"

        .ifdef SURL_TO_LANGCARD
        .segment "LC"
        .else
        .segment "LOWCODE"
        .endif

;char __fastcall__ simple_serial_getc(void) {

_simple_serial_getc:
        lda     #<tmp2
        ldx     #>tmp2
        jsr     _ser_get
        cmp     #SER_ERR_NO_DATA
        beq     _simple_serial_getc
        lda     tmp2
        ldx     #$00
        rts

;void __fastcall__ simple_serial_puts(const char *buf) {

_simple_serial_puts:
        jsr     pushax
        jsr     _strlen
        jmp     _simple_serial_write


simple_serial_compute_ptr_end:
        sta     ptr3
        stx     ptr3+1
        jsr     popax
        sta     ptr4
        stx     ptr4+1
        clc
        adc     ptr3            ; set ptr3 to end
        sta     ptr3
        lda     ptr4+1
        adc     ptr3+1
        sta     ptr3+1
        rts

        .ifdef  SIMPLE_SERIAL_DUMP
_simple_serial_dump:
        jsr     simple_serial_compute_ptr_end
        lda     #$17            ; SURL_METHOD_DUMP
        jsr     _serial_putc_direct
        lda     #$0A            ; \n
        jsr     _serial_putc_direct
        jsr     popa            ; dump ID
        jsr     _serial_putc_direct

        lda     ptr4+1          ; Dump start
        jsr     _serial_putc_direct
        lda     ptr4
        jsr     _serial_putc_direct

        lda     ptr3+1          ; Dump end
        jsr     _serial_putc_direct
        lda     ptr3
        jsr     _serial_putc_direct
        jmp     write_again
        .endif

; void __fastcall__ simple_serial_write(const char *ptr, size_t nmemb) {
_simple_serial_write:
        jsr     simple_serial_compute_ptr_end
write_again:
        ldy     #$00
        lda     (ptr4),y
        jsr     _serial_putc_direct

        inc     ptr4
        bne     :+
        inc     ptr4+1
:       lda     ptr4
        cmp     ptr3
        bne     write_again
        ldx     ptr4+1
        cpx     ptr3+1
        bne     write_again
        rts

_simple_serial_flush:
        jsr     _simple_serial_getc_with_timeout
        cpx     #$FF
        bne     _simple_serial_flush
        rts
