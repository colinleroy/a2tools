update_title:
        tya                     ; Backup Y
        pha

        lda     title_addr      ; gotoxy
        sta     BASL
        lda     title_addr+1
        sta     BASH
        lda     #0
:       sta     CH              ; Ignore OURCH here, it's not a problem.

        ldx     numcols         ; Reset char counter

st:     lda     ser_status      ; 21    Check serial
        and     has_byte        ; 24
        beq     st

                                ; cycles since fetch, worst case,
                                ; must be under 85 for serial access
dt:     lda     ser_data        ; 4
        ora     #$80            ; 6

        bit     _is_iie         ; 10
        bmi     :+              ; 12
        cmp     #$E0            ; 14            Should we uppercase?
        bcc     :+              ; 16
        and     uppercasemask   ; 20
:

        pha                     ; 23            Local, stripped-down version of putchardirect
        lda     CH              ; 26
        bit     _is_iie         ; 30
        bpl     put             ; 32
        bit     RD80VID         ; 36           In 80 column mode?
        bpl     put             ; 38           No, just go ahead
        lsr                     ; 40           Div by 2
        bcs     put             ; 42           Odd cols go in main memory
        bit     HISCR           ; 46           Assume SET80COL
put:    tay                     ; 48

        pla                     ; 51
        sta     (BASL),Y        ; 56

        bit     LOWSCR          ; 60           Doesn't hurt in 40 column mode

        inc     CH              ; 65           Can't wrap line!

        dex                     ; 67
        bne     st              ; 69

        pla                     ; 72           Restore Y
        tay                     ; 74

st2:    lda     ser_status      ;              Wait for next audio byte
        and     has_byte
        beq     st2
dt2:    ldx     ser_data
desttitle:
        JMP_NEXT_6
