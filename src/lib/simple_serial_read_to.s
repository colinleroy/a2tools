;
; Ullrich von Bassewitz, 06.08.1998
; Colin Leroy-Mira <colin@colino.net>, 2023
;
; void __fastcall__ simple_serial_read_to(char *start, char *end);
;

        .export         _simple_serial_read_to
        .import         popax, _ser_get
        .importzp       ptr1, ptr3, ptr4

SER_ERR_NO_DATA  := $06

_simple_serial_read_to:
        sta ptr3    ;save end to ptr3
        stx ptr3+1

save_start:
        jsr popax   ;pop start to ptr4
        sta ptr4
        stx ptr4+1

again:
        jsr _ser_get            ;fetch byte into ptr1
        cmp #SER_ERR_NO_DATA    ;nothing received
        beq again

        lda (ptr1, x)           ;get the byte (X is 0 out of ser_get)
        sta (ptr4, x)           ;store the byte

        inc ptr4
        bne checkend            ;check if we're at the end
        inc ptr4+1              ;increment start high byte

checkend:
        lda ptr4
        cmp ptr3                ;compare to ptr4
        bne again               ;low bytes differ, continue reading
        lda ptr4+1
        cmp ptr3+1
        bne again               ;high bytes differ, continue reading
; Done

L9:     rts
