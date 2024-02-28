;
; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .export         _simple_serial_read

        .import         simple_serial_compute_ptr_end
        .import         _ser_get

        .importzp       ptr3, ptr4
        .include        "apple2.inc"
        .include        "ser-error.inc"

        .ifdef SURL_TO_LANGCARD
        .segment "LC"
        .else
        .segment "LOWCODE"
        .endif

;void __fastcall__ simple_serial_read(char *ptr, size_t nmemb) {
_simple_serial_read:
        jsr     simple_serial_compute_ptr_end

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
