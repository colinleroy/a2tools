;
; Ullrich von Bassewitz, 06.08.1998
; Colin Leroy-Mira <colin@colino.net>, 2023
;
; void __fastcall__ dputcxy (unsigned char x, unsigned char y, char c);
; void __fastcall__ dputc (char c);
;

        .include        "apple2.inc"

bell:
        bit     $C082
        jsr     $FF3A           ; BELL
        bit     $C080
        rts

.ifndef AVOID_ROM_CALLS

        .export         _dputcxy, _dputc
        .import         _scrollup_one, gotoxy, FVTABZ, _cputc
        .import         uppercasemask, machinetype

_dputcxy:
        pha                     ; Save C
        jsr     gotoxy          ; Call this one, will pop params
        pla                     ; Restore C and run into _dputc

_dputc:
        cmp    #$07
        bne    :+
        jmp     bell
:       pha
        jsr     _cputc
        pla
        cmp     #$0D            ; Don't scroll if \r
        beq     noscroll
        lda     WNDTOP          ; Don't scroll if not at first line
        cmp     CV
        bne     noscroll

        bit     machinetype
        bpl     get40
        bit     RD80VID
        bpl     get40
        lda     OURCH              ; Don't scroll if first line but not first char
        jmp     check_scroll
get40:
        lda     CH
check_scroll:
        bne     noscroll

        jsr     _scrollup_one
        lda     WNDBTM
        sec
        sbc     #1
        sta     CV
        jsr     FVTABZ

noscroll:
        rts

.else

        .export         _dputcxy, _dputc
        .import         gotoxy, FVTABZ
        .import         putchar, uppercasemask
        .import         _scrollup_one
        .import         machinetype

        .code

; Plot a character - also used as internal function

dnewline:
        inc     CV              ; Bump to next line
        lda     CV
        cmp     WNDBTM          ; Are we at bottom?
        bcc     :+
        dec     CV              ; Yes, decrement
        jsr     _scrollup_one   ; and scroll
        lda     CV
:       jmp     FVTABZ

special_chars:
        cmp     #$0D            ; Test for \r = carrage return
        beq     left
        cmp     #$0A            ; Test for \n = line feed
        beq     dnewline
        cmp     #$08            ; Test for backspace
        beq     backspace
        cmp     #$07            ; Test for bell
        beq     bell
        bne     invert          ; Back to standard codepath

_dputcxy:
        pha                     ; Save C
        jsr     gotoxy          ; Call this one, will pop params
        pla                     ; Restore C and run into _dputc

_dputc:
        cmp     #$0E            ; Test for special chars <= \r
        bcc     special_chars   ; Skip other tests if possible
invert: eor     #$80            ; Invert high bit

        bit     machinetype
        bmi     dputdirect
        cmp     #$E0            ; Test for lowercase
        bcc     dputdirect
        and     uppercasemask

dputdirect:
        jsr     putchar

        bit     machinetype
        bpl     inc40
        bit     RD80VID
        bpl     inc40
        inc     OURCH
        lda     OURCH
        jmp     cmpw
inc40:
        inc     CH              ; Bump to next column
        lda     CH
cmpw:
        cmp     WNDWDTH
        bcs     :+
        rts

:       jsr     dnewline
left:
        lda     #$00
        bit     machinetype
        bpl     zero40
        bit     RD80VID
        bpl     zero40
        sta     OURCH
        jmp     out
zero40:
        sta     CH
out:
        rts

backspace:
        bit     machinetype
        bpl     ld40
        bit     RD80VID
        bpl     ld40
        lda     OURCH
        jmp     chk
ld40:
        lda     CH              ; are we at col 0
chk:
        bne     decrh           ; no, we can decrement 
        ldy     CV              ; yes,
        cpy     WNDTOP          ; so are we at row 0
        beq     bout            ; yes, do nothing
        dey                     ; no, decr row
        sty     CV              ; store it
        ldy     WNDWDTH         ; prepare CH for decr

        bit     machinetype
        bpl     stch
        bit     RD80VID
        bpl     stch
        sty     OURCH
        jmp     upcv
stch:   sty     CH
upcv:   lda     CV
        jsr     FVTABZ

decrh:
        bit     machinetype
        bpl     decch
        bit     RD80VID
        bpl     decch
        dec     OURCH
        rts
decch:
        dec     CH
bout:   rts

.endif
