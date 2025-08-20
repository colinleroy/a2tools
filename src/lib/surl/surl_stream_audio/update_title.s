update_title:
        sty     tmp1            ; Backup Y

        lda     title_addr      ; gotoxy
        sta     BASL
        lda     title_addr+1
        sta     BASH
        lda     #0
        sta     CH              ; Ignore OURCH here, it's not a problem.

        ldx     numcols         ; Reset char counter

st:     lda     ser_status      ;               Check serial
        and     has_byte        ;
        beq     st
dt:     lda     ser_data        ;
        ora     #$80            ;

                                ; cycles since fetch, worst case,
                                ; must be under 85 for serial access

        .ifndef __APPLE2ENH__
        bit     _is_iie         ;
        bmi     :+              ;
        cmp     #$E0            ; Should we uppercase?
        bcc     :+              ;
        and     uppercasemask   ;
:       .endif
        sta     tmp2

        ldy     CH              ;

        sec                     ; Assume main memory
        bit     _is_iie         ;
        bpl     put             ;
        bit     RD80VID         ; In 80 column mode?
        bpl     put             ; No, just go ahead
        tya                     ;
        lsr                     ;
        tay                     ;
        bcs     put             ; Odd cols go in main memory
        php                     ;
        sei                     ; No interrupts while writing AUX
        bit     HISCR           ; Assume SET80COL
put:
        lda     tmp2
        sta     (BASL),Y        ;
        bcs     :+              ;
        bit     LOWSCR          ; Doesn't hurt in 40 column mode
        plp                     ;

:       inc     CH              ; Can't wrap line!

        dex                     ; More chars?
        bne     st              ;

        ldy     tmp1            ; Restore Y

st2:    lda     ser_status      ; Wait for next audio byte
        and     has_byte
        beq     st2
dt2:    ldx     ser_data
        STORE_TARGET_3
        JMP_NEXT_6
