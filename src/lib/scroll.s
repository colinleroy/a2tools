;
; From Apple ][e ROM
; https://ia800708.us.archive.org/35/items/Apple_IIe_Technical_Reference_Manual/Apple_IIe_Technical_Reference_Manual_text.pdf
; pages 366 - 368
;

        .export         _scrolldown_one, _scrollup_one, _scrolldown_n, _scrollup_n
        .import         VTABZ
        .include        "apple2.inc"

        .bss
NLINES: .res 1
        .code

BAS2L  := $2A
BAS2H  := $2B
TXTPAGE1 := $C054
TXTPAGE2 := $C055
TEMP1    := $778+3
SEV1     := $CB06

_scrolldown_n:
        sta     NLINES          ;save A (# of lines)
dnagain:
        ldy     #0              ;direction = down
        jsr     scrollit        ;go do scroll
        dec     NLINES
        bne     dnagain
        rts

_scrollup_n:
        sta     NLINES          ;save A (# of lines)
upagain:
        ldy     #1              ;direction = up
        jsr     scrollit        ;go do scroll
        dec     NLINES
        bne     upagain
        rts

_scrolldown_one:
        ldy     #0              ;direction = down
        beq     scrollit        ;go do scroll

_scrollup_one:
        ldy     #1              ;direction = up

scrollit:
        txa                     ;save X
        pha
        sty     TEMP1           ;save direction
        lda     WNDWDTH         ;get width of screen window
        pha                     ;save original width
        bit     RD80VID         ;in 40 or 80 columns?
        bpl     getstl          ;=>40, determine starting line
        sta     SET80COL        ;make sure this is enabled
        lsr     a               ;divide by 2 for 80 columns index
        tax                     ;and save
        lda     WNDLFT          ;test oddity of right edge
        lsr     a               ;by rotating low bit into carry
        clv                     ;V=0 if left edge even
        bcc     chkrt           ;check right edge
        bit     SEV1            ;V=1 if left edge odd
chkrt:
        rol     a               ;restore WNDLFT
        eor     WNDWDTH         ;get oddity of right edge
        lsr     a               ;C=1 if right edge even
        bvs     getst           ;if odd left, don't dex
        bcs     getst           ;if even right, don't dex
        dex                     ;need one less
getst:
        stx     WNDWDTH         ;save window width
        lda     RD80VID         ;N=1 if 80 columns
getstl:
        php                     ;save N,Z,V
        ldx     WNDTOP          ;assume scroll from top
        tya                     ;up or down ?
        bne     setdbas         ;up
        ldx     WNDBTM          ;down, start scrolling at bottom
        dex                     ;really need one less
setdbas:
        txa                     ;get current line
        jsr     VTABZ           ;calculate base with window width
scrlin:
        lda     BASL            ;current line is destination
        sta     BAS2L
        lda     BASH
        sta     BAS2H

        lda     TEMP1           ;test direction
        beq     scrldn          ;=>do the downer
        inx                     ;do next line
        cpx     WNDBTM          ;done yet?
        bcs     scrll3          ;yup, all done
setsrc:
        txa                     ;set new line
        jsr     VTABZ           ;get base for current line
        ldy     WNDWDTH         ;get width for scroll
        plp                     ;get status for scroll
        php                     ;N=1 if 80 columns
        bpl     skprt           ;=>only do 40 columns
        lda     TXTPAGE2        ;scroll aux page first (even bytes)
        tya                     ;test Y
        beq     scrlft          ;if 0, only scroll one byte
scrleven:
        lda     (BASL),y
        sta     (BAS2L),y
        dey
        bne     scrleven        ;do all but last even byte
scrlft:
        bvs     skplft          ;odd left edge, skip this byte
        lda     (BASL),y
        sta     (BAS2L),y
skplft:
        lda     TXTPAGE1        ;now do main page (odd bytes)
        ldy     WNDWDTH         ;restore width
        bcs     skprt           ;even right edge, skip this byte
scrllodd:
        lda     (BASL),y
        sta     (BAS2L),y
skprt:
        dey
        bpl     scrllodd
        bmi     scrlin          ;=>always scroll next line
scrldn:
        dex                     ;do next line
        cpx     WNDTOP          ;done yet?
        bpl     setsrc          ;nope, not yet
scrll3:
        plp                     ;pull status off stack
        pla                     ;restore window width
        sta     WNDWDTH
        jsr     clrline        ;clear current line //TODO
        lda     CV
        jsr     VTABZ           ;restore original cursor line
        pla                     ;and X
        tax
        rts                     ;done!!!

clrline:
        ldy     #0              ;start at left
        beq     xgseolz         ;clear to end of line

xgseolz:
        lda     INVFLG          ;mask blank
        and     #$80            ;with high bit
        ora     #$20            ;make it a blank
        bit     RD80VID         ;is it 80 columns?
        bmi     clr80           ;yes, do quick clear
clr40:
        sta     (BASL),y
        iny
        cpy     WNDWDTH
        bcc     clr40
        rts

clrhalf:
        stx     BAS2L
        ldx     #$D8
        ldy     #20
        lda     INVFLG
        and     #$A0
        jmp     clr2

clr80:
        stx     BAS2L
        pha
        tya
        pha
        sec
        sbc     WNDWDTH
        tax
        tya
        lsr     a
        tay
        pla
        eor     WNDLFT
        ror     a
        bcs     clr0
        bpl     clr0
        iny
clr0:
        pla
        bcs     clr1
clr2:
        bit     TXTPAGE2
        sta     (BASL),Y
        bit     TXTPAGE1
        inx
        beq     clr3
clr1:
        sta     (BASL),y
        iny
        inx
        bne     clr2
clr3:
        ldx     BAS2L
        sec
        rts
