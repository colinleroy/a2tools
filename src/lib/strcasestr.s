;
; Ullrich von Bassewitz, 11.12.1998
;
; char* strcasestr (const char* haystack, const char* needle);
;

        .export         _strcasestr
        .import         popptr1, return0, _tolower, ctypemaskdirect
        .importzp       ptr1, ptr2, ptr3, ptr4, tmp1, tmp2, tmp3, tmp4
        .include        "ctype.inc"

tolower_safe:
        pha                     ; save char
        jsr     ctypemaskdirect ; get character classification
        and     #CT_UPPER       ; upper case char?
        beq     @L1             ; jump if no
        pla                     ; restore char
        adc     #<('a'-'A')     ; make lower case char (ctypemaskdirect ensures carry clear)
        rts
@L1:    pla                     ; restore char
        rts

_strcasestr:
        sta     ptr2            ; Save needle
        stx     ptr2+1
        sta     ptr4            ; Setup temp copy for later

        jsr     popptr1         ; Get haystack to ptr1

; If needle is empty, return haystack

        ; ldy     #$00            Y=0 guaranteed by popptr1
        lda     (ptr2),y        ; Get first byte of needle
        beq     @Found          ; Needle is empty --> we're done

; Search for the beginning of the string (this is not an optimal search
; strategy [in fact, it's pretty dumb], but it's simple to implement).

        jsr     tolower_safe    ; Lowercase
        sta     tmp1            ; Save start of needle
@L1:    lda     (ptr1),y        ; Get next char from haystack
        beq     @NotFound       ; Jump if end

        jsr     tolower_safe    ; Lowercase
        cmp     tmp1            ; Start of needle found?
        beq     @L2             ; Jump if so
        iny                     ; Next char
        bne     @L1
        inc     ptr1+1          ; Bump high byte
        bne     @L1             ; Branch always

; We found the start of needle in haystack

@L2:    tya                     ; Get offset
        clc
        adc     ptr1
        sta     ptr1            ; Make ptr1 point to start
        bcc     @L3
        inc     ptr1+1

; ptr1 points to the start of needle in haystack now. Setup temporary pointers for the
; search. The low byte of ptr4 is already set.

@L3:    sta     ptr3
        lda     ptr1+1
        sta     ptr3+1
        lda     ptr2+1
        sta     ptr4+1
        ldy     #1              ; First char is identical, so start on second

; Do the compare

@L4:    lda     (ptr4),y        ; Get char from needle
        beq     @Found          ; Jump if end of needle (-> found)

        jsr     tolower_safe    ; Lowercase
        sta     tmp2

        lda     (ptr3),y        ; Compare with haystack

        jsr     tolower_safe    ; Lowercase
        cmp     tmp2
        bne     @L5             ; Jump if not equal
        iny                     ; Next char
        bne     @L4
        inc     ptr3+1
        inc     ptr4+1          ; Bump hi byte of pointers
        bne     @L4             ; Next char (branch always)

; The strings did not compare equal, search next start of needle

@L5:    ldy     #1              ; Start after this char
        bne     @L1             ; Branch always

; We found the start of needle

@Found: lda     ptr1
        ldx     ptr1+1
        rts

; We reached end of haystack without finding needle

@NotFound:
        jmp     return0         ; return NULL
