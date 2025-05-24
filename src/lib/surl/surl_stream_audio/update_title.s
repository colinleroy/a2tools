update_title:
        tya                     ; Backup Y
        pha

        lda     title_addr      ; gotoxy
        sta     BASL
        lda     title_addr+1
        sta     BASH
        lda     #0
        sta     CH

        ldx     numcols         ; Reset char counter

st:     lda     ser_status      ; 21    Check serial
        and     has_byte        ; 24
        beq     st

                                ; 6502  65C02 (cycles since fetch, worst case,
                                ;              must be under 85 for serial access)
dt:     lda     ser_data        ; 4     4
        ora     #$80            ; 6     6

        .ifndef __APPLE2ENH__
        cmp     #$E0            ; 8            Should we uppercase?
        bcc     :+              ; 10/11
        and     uppercasemask   ; 13
:
        .endif

        pha                     ; 16    9      Local, stripped-down version of putchardirect
        .ifdef  __APPLE2ENH__   ;              to save 12 jsr+rts cycles
        lda     CH              ;       12
        bit     RD80VID         ;       16     In 80 column mode?
        bpl     put             ;       18/19  No, just go ahead
        lsr                     ;       20     Div by 2
        bcs     put             ;       22/23  Odd cols go in main memory
        bit     HISCR           ;       26     Assume SET80COL
put:    tay                     ;       28
        .else
        ldy     CH              ; 19
        .endif

        pla                     ; 22    31
        sta     (BASL),Y        ; 28    36

        .ifdef  __APPLE2ENH__
        bit     LOWSCR          ;       40     Doesn't hurt in 40 column mode
        .endif

        inc     CH              ; 33    45     Can't wrap line!

        dex                     ; 35    47
        bne     st              ; 37/38 49/50

        pla                     ; 40    52     Restore Y
        tay                     ; 42    54

st2:    lda     ser_status      ; 21    Check serial
        and     has_byte        ; 24
        beq     st2
dt2:    ldx     ser_data
desttitle:
        JMP_NEXT_6
