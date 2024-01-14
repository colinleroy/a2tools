;
; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .export         _simple_serial_puts
        .export         _simple_serial_getc
        .export         _simple_serial_getc_immediate
        .export         _simple_serial_write
        .export         _simple_serial_read
        .export         _simple_serial_flush

        .import         _simple_serial_getc_with_timeout
        .import         _ser_get, _ser_put, _strlen
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

;int __fastcall__ simple_serial_getc_immediate(void) {
_simple_serial_getc_immediate:
        lda     #<tmp2
        ldx     #>tmp2
        jsr     _ser_get
        cmp     #SER_ERR_NO_DATA
        beq     no_char
        lda     tmp2
        ldx     #$00
        rts
no_char:
        lda     #$FF
        tax
        rts

;void __fastcall__ simple_serial_puts(const char *buf) {

_simple_serial_puts:
        jsr     pushax
        jsr     _strlen
        jmp     _simple_serial_write


compute_ptr_end:
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

; void __fastcall__ simple_serial_write(const char *ptr, size_t nmemb) {

_simple_serial_write:
        jsr     compute_ptr_end
write_again:
        ldy     #$00
        lda     (ptr4),y
        jsr     _ser_put
        cmp     #SER_ERR_OVERFLOW
        beq     write_again

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

;void __fastcall__ simple_serial_read(char *ptr, size_t nmemb) {

_simple_serial_read:
        jsr     compute_ptr_end

        lda     #$00
        beq     check_bound
read_again:
        lda     ptr4
read_again_aok:
        ldx     ptr4+1
read_again_axok:
        jsr     _ser_get
        cmp     #SER_ERR_NO_DATA
        beq     read_again

        inc     ptr4
        bne     check_bound
        inc     ptr4+1

check_bound:
        lda     ptr4
        cmp     ptr3
        bne     read_again_aok
        ldx     ptr4+1
        cpx     ptr3+1
        bne     read_again_axok

        rts

_simple_serial_flush:
        jsr     _simple_serial_getc_with_timeout
        cpx     #$FF
        bne     _simple_serial_flush
        rts
