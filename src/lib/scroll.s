;
; From Apple ][e ROM
; https://ia800708.us.archive.org/35/items/Apple_IIe_Technical_Reference_Manual/Apple_IIe_Technical_Reference_Manual_text.pdf
; pages 366 - 368
;

        .export         _scrolldown_one, _scrollup_one, _scrolldown_n, _scrollup_n
        .import         FVTABZ
        .ifndef AVOID_ROM_CALLS
        .import         COUT
        .endif
        .include        "apple2.inc"

        .bss

NLINES: .res 1

BAS2L  := $2A
BAS2H  := $2B
TXTPAGE1 := $C054
TXTPAGE2 := $C055
TEMP1    := $778+3
SEV1     := $CB06

_scrolldown_n:
        sta     NLINES          ;save A (# of lines)
dnagain:
        jsr     _scrolldown_one
        dec     NLINES
        bne     dnagain
        rts

        .segment "LOWCODE"

_scrollup_n:
        sta     NLINES          ;save A (# of lines)
upagain:
        jsr     _scrollup_one
        dec     NLINES
        bne     upagain
        rts

.ifdef __APPLE2ENH__

.ifndef AVOID_ROM_CALLS

_scrolldown_one:
        lda     RD80VID         ;in 40 or 80 columns?
        bpl     :+
        lda     #($16|$80)      ; 80col firmware's Ctrl-V
        jmp     COUT
:       rts

_scrollup_one:
        bit     $C082
        jsr     $FC70           ; SCROLL
        bit     $C080
        rts

.else

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
        jsr     FVTABZ          ;calculate base with window width
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
        jsr     FVTABZ          ;get base for current line
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
        jsr     clrline        ;clear current line
        lda     CV
        jsr     FVTABZ          ;restore original cursor line
        pla                     ;and X
        tax
        rts                     ;done!!!

clrline:
        ldy     #0              ;start at left

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

.endif  ; AVOID_ROM_CALLS

.else   ; __APPLE2__

_scrolldown_one:
        ldy     #0              ;direction = down
        beq     scrollit        ;go do scroll

_scrollup_one:
        ldy     #1              ;direction = up

scrollit:
        sty     TEMP1           ;save direction
        cpy     #1
        beq     initup
        lda     WNDBTM
        sec
        sbc     #1
        jmp     start

initup:
        lda     WNDTOP
start:
        pha
        jsr     FVTABZ
        
scrl1:  lda     BASL
        sta     BAS2L
        lda     BASH
        sta     BAS2H

        lda     TEMP1           ;remember direction
        bne     contup
        
        pla
        sec
        sbc     #1              ;down
        cmp     WNDTOP
        bmi     scrl3
        bpl     cont

contup:
        pla
        adc     #1
        cmp     WNDBTM
        bcs     scrl3
cont:
        pha
        jsr     FVTABZ

        ldy     WNDWDTH
        dey
scrl2:  lda     (BASL),y
        sta     (BAS2L),y
        dey
        bpl     scrl2

        bmi     scrl1

scrl3:  ldy     #0
        lda     #' '|$80
cleol2: sta     (BASL),y
        iny
        cpy     WNDWDTH
        bcc     cleol2
        rts

.endif
