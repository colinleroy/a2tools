;
; Colin Leroy-Mira <colin@colino.net>, 2023
;

        .export         _simple_serial_getc_immediate

        .import         _ser_get

        .importzp       tmp2

        .include        "apple2.inc"
        .include        "ser-error.inc"

        .ifdef SURL_TO_LANGCARD
        .segment "LC"
        .else
        .segment "LOWCODE"
        .endif

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
