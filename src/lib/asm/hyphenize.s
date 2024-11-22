        .export       _hyphenize
        .import       _strlen
        .import       popax, pushax, decax4, tosaddax
        .importzp     ptr1, tmp1

; void __fastcall__ hyphenize(char *str, int len)
_hyphenize:
        sta       tmp1        ; Backup len
        stx       tmp1+1

        jsr       popax       ; Pop and save str
        jsr       pushax
        jsr       _strlen
        cpx       tmp1+1      ; Check length
        bcc       @no_hyphen
        cmp       tmp1
        bcc       @no_hyphen

        lda       tmp1        ; Compute len - 4
        ldx       tmp1+1
        jsr       decax4
        jsr       tosaddax    ; Add to str

        sta       ptr1        ; Pointerize str
        stx       ptr1+1

        lda       #$00        ; Put 0 at end
        ldy       #4
        sta       (ptr1),y
        lda       #'.'        ; Put 3 dots before
        dey
:       sta       (ptr1),y
        dey
        bne       :-
        rts
@no_hyphen:
        jmp       popax       ; Discard str from TOS
