update_title:
        php                     ; No interrupts while writing AUX
        sei                     ; so disable for the whole title update

        bit     _is_iie         ; Check if 40 columns and route
        bpl     update_title_40 ; accordingly - faster than at each char
        bit     RD80VID         ; In 80 column mode?
        bmi     update_title_80 ; Yes

update_title_40:
        sty     tmp1            ; Backup Y

        lda     title_addr      ; gotoxy
        sta     BASL
        lda     title_addr+1
        sta     BASH
        lda     #0
        sta     CH              ; Ignore OURCH here, it's not a problem.

        ldx     numcols         ; Reset char counter

st40:   lda     ser_status      ;               Check serial
        and     has_byte        ;
        beq     st40
dt40:   lda     ser_data        ;
        ora     #$80            ;

                                ; cycles since fetch, worst case,
                                ; must be under 85 for serial access

        cmp     #$E0            ; Should we uppercase?
        bcc     :+              ;
        and     uppercasemask   ;
:
        ldy     CH              ;
        sta     (BASL),Y        ;
        inc     CH              ; Can't wrap line!

        dex                     ; More chars?
        bne     st40            ;

        jmp     update_title_done

update_title_80:
        sty     tmp1            ; Backup Y

        lda     title_addr      ; gotoxy
        sta     BASL
        lda     title_addr+1
        sta     BASH
        lda     #0
        sta     CH              ; Ignore OURCH here, it's not a problem.

        ldx     numcols         ; Reset char counter

st80:   lda     ser_status      ;               Check serial
        and     has_byte        ;
        beq     st80
dt80:   lda     ser_data        ;

                                ;    cycles since fetch, worst case,
                                ;    must be under 85 for serial access
        ora     #$80            ; 2
        sta     tmp2            ; 5

        ldy     CH              ; 8

        sec                     ; 10 Assume main memory
        bit     RD80VID         ; 14 In 80 column mode?
        bpl     put80           ; 16 No, just go ahead
        tya                     ; 18
        lsr                     ; 20
        tay                     ; 22
        bcs     put80           ; 24 Odd cols go in main memory
        bit     HISCR           ; 28 Assume SET80COL
put80:
        lda     tmp2            ; 31
        sta     (BASL),Y        ; 37
        bit     LOWSCR          ; 41

        inc     CH              ; 44 Can't wrap line!
        dex                     ; 46 More chars?
        bne     st80            ;

update_title_done:
        ldy     tmp1            ; Restore Y
st:     lda     ser_status      ; Wait for next audio byte
        and     has_byte
        beq     st
dt:     ldx     ser_data
        plp
        STORE_TARGET_3
        JMP_NEXT_6
