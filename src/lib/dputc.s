;
; Ullrich von Bassewitz, 06.08.1998
; Colin Leroy-Mira <colin@colino.net>, 2023
;
; void __fastcall__ dputcxy (unsigned char x, unsigned char y, char c);
; void __fastcall__ dputc (char c);
;

        .export         _dputcxy, _dputc
        .export         dputdirect, dnewline
        .import         gotoxy, VTABZ, putchar
        .import         _scrollup, _scrolldn

        .include        "apple2.inc"

        .segment        "ONCE"

        .code

; Plot a character - also used as internal function

_dputcxy:
        pha                     ; Save C
        jsr     gotoxy          ; Call this one, will pop params
        pla                     ; Restore C and run into _dputc

_dputc:
        cmp     #$0D            ; Test for \r = carrage return
        beq     left
        cmp     #$0A            ; Test for \n = line feed
        beq     dnewline
        cmp     #$08            ; Test for backspace
        beq     backspace
        cmp     #$07            ; Test for bell
        beq     bell
        eor     #$80            ; Invert high bit

dputdirect:
        jsr     putchar
        inc     CH              ; Bump to next column
        lda     CH
        cmp     WNDWDTH
        bcc     :+
        jsr     dnewline
left:   lda     #$00            ; Goto left edge of screen
        sta     CH
:       rts

bell:
        bit     $C082
        jsr     $FBE4           ; BELL2
        bit     $C080
        rts

backspace:
        lda     CH
        cmp     WNDLFT          ; are we at col 0
        bne     decrh           ; no, we can decrement 
        ldy     CV              ; yes,
        cmp     WNDTOP          ; so are we at row 0
        beq     :+              ; yes, do nothing
        dey                     ; no, decr row
        sty     CV              ; store it
        ldy     WNDWDTH         ; prepare CH for decr
        sty     CH
decrh:  dec     CH
:       rts

dnewline:
        inc     CV              ; Bump to next line
        lda     CV
        cmp     WNDBTM          ; Are we at bottom?
        bcc     :+
        dec     CV              ; Yes, decrement
        jsr     _scrollup       ; and scroll
        lda     CV
:       jmp     VTABZ
