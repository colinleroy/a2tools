; HGR Toolbox
.export		Font
.export		Tile
.import		popa

.importzp   sp, sreg, regsave, regbank
;------------------------------------------------
; Constants
;------------------------------------------------

.include "apple2.inc"
;------------------------------------------------
; Constants
;------------------------------------------------
       	HgrLo   	= $E5   ; Low  byte Pointer to screen destination
        HgrHi   	= $E6   ; High byte Pointer to screen destination
        String  	= $F0
        TmpLo   	= $F5   ; Low  byte Working pointer to screen byte
        TmpHi   	= $F6   ; High byte Working pointer to screen byte
        TilePtrLo   = $F7   ;                                          
        TilePtrHigh  = $F8   ;   
        TopHi  		= $FD
.segment "HGR"
;------------------------------------------------
.segment "CODE"
;============================
.export	_init_hgr
.proc	_init_hgr
 	sta	TXTCLR
 	sta	HIRES
 	sta	MIXSET
	lda	#$20				;Hgr page 1
	sta	HgrHi
	lda	#0
	sta	HgrLo
	sta	_cursorX
	sta	_cursorY
	rts
.endproc
;============================
.export	_set_text
.proc	_set_text
	bit	$c051
	rts
.endproc
;============================
.export _set_row
.proc	_set_row
	sta	_cursorY
	tax	
	lda	HgrLoY,x			;set TmpLo,High from HgrxxY table , index _cursorY
	sta	TmpLo
	lda	HgrHiY,x
	clc
	adc	HgrHi
	sta	TmpHi
	rts
.endproc
;============================
;set_colrow(char col, char row)
.export	_set_colrow
.proc	_set_colrow
	sta	_cursorY
	jsr	popa
	sta	_cursorX
	sta	TmpLo
	ldy	_cursorY
	ldx	_cursorX
	lda	HgrLoY,y				; HgrLoy[ row ]
	clc
	adc	TmpLo					; add column
	sta	TmpLo
	lda	HgrHiY,y				; HgrHiY[ row ]
	clc							;  could optimize this into
	adc	HgrHi					;  single ORA HgrHi
	sta	TmpHi
	rts
.endproc
;============================
;.export _set_colrowYX
;.proc _set_colrowYX
;	jsr	_set_cursor_row
;	clc
;	tya
;	adc	TmpLo
;	sta	TmpLo
;	rts
;.endproc
;=======================
; set_cursor_row(int row);
.export	_set_cursor_row
.proc	_set_cursor_row
	lda	HgrLoY,x			; HgrLoY[row]
	sta	TmpLo
	lda	HgrHiY,x
	clc
	adc	HgrHi
	sta	TmpHi
	rts
.endproc
;=======================
.export _draw_string
.proc _draw_string
    sta String 		; parameter a = lAddr x = hAddr
	stx String+1
    ldy #0
ds1:
	lda (String),y
	beq	ds2			; null byte done
	jsr	_draw_char_col
	cpy	#40			; col < 40 ?
	bcc	ds1
ds2:
	rts
;draw_char_col( char c, int col );
.export _draw_char_col
_draw_char_col:
	pha
	and		#$e0	; %11100000 extract high 3bit
	clc
	rol				; msb is in carry
	rol				;     is in bit0
	rol				;     is in bit1
	rol				;     is in bit2
	sta		index+1	; save index High
	pla
	asl				; * 2
	asl				; * 4
	asl				; * 8
	sta		index	;save index Lo		
	clc
	adc		#<Font
	sta		_LoadFont+1	; self mod
	lda		index+1		;
	adc		#>Font
	sta		_LoadFont+2	;
;	rol
;	rol
;	rol
;	tax
;	and	#$f8
;	sta	_LoadFont+1
;	txa
;	and	#3
;	rol
;	adc	#>Font		;+= FontHi; Carry = 0 since s+0 from above
;	sta	_LoadFont+2
_DrawChar1:
	ldx	TmpHi
	stx	TopHi
_DrawChar:
	ldx	#0
_LoadFont:
	lda	Font,x
;	and #$2a		; color mask
	sta	(TmpLo),y	; screen[col] = A
	clc
	lda	TmpHi
	adc	#4			; screen += 0x400
	sta	TmpHi
	inx
	cpx	#8
	bne	_LoadFont
IncCursorCol:
	iny
	sty	_cursorX	; my
	ldx	TopHi		; Move cursor back to top of scanline
	stx	TmpHi
	rts
index:	.res		2
.endproc
.export		_draw_tile
.proc	   _draw_tile
    ; tile index passed in A
    jsr     setTilePtr 

    ; calculate screen pointer
    ; is in Hgr Lo, High

	lda		_cursorY
	jsr		_set_row
    jsr     move8line

    ; next Row

    inc     _cursorY
    lda     _cursorY
    jsr     _set_row

    jsr     move8line
    rts
.endproc
;
;
.proc       move8line
    clc
	lda		TmpLo
	adc		_cursorX
	sta		TmpLo				;add X pos
    ldx     #8
drawLoop:
    ldy     #0
    lda     (TilePtrLo),y
    sta     (TmpLo),y
    ldy     #1
    lda     (TilePtrLo),y
    sta     (TmpLo),y
    ldy		#2
    lda     (TilePtrLo),y
    sta     (TmpLo),y
	ldy		#3
    lda     (TilePtrLo),y
    sta     (TmpLo),y
    ; next line
    clc
    lda     TilePtrLo
    adc     #4
    sta     TilePtrLo
    lda     TilePtrHigh
    adc     #0
    sta     TilePtrHigh

    clc
    lda     TmpHi
    adc     #4
    sta     TmpHi

    dex
    bne     drawLoop
    rts
.endproc
;============================
; Tile id must be < 16
.proc   setTilePtr
	sta		tileCode
    ldy     #0
    sty     offset+1
	lda		tileCode
    and     #$fc        ; if id > 5 then overflow
    beq     skip1
    inc     offset+1
	lda		tileCode
    and     #$f8
    beq     skip1
    inc     offset+1
skip1:
	lda		tileCode
    asl
    asl
    asl
    asl
    asl
    asl
    sta     offset
    clc
    lda     #<Tile
    adc     offset
    sta     TilePtrLo
    lda     #>Tile
    adc     offset+1
    sta     TilePtrHigh
    rts
tileCode:	.byte	0
offset:     .word  0
.endproc

;=======================
.export     _cursorY
.export     _cursorX
_cursorY:   .byte 0
_cursorX:   .byte 0
;=============================================================================
HgrLoY:
        .byte $00,$80,$00,$80,$00,$80,$00,$80
        .byte $28,$A8,$28,$A8,$28,$A8,$28,$A8
        .byte $50,$D0,$50,$D0,$50,$D0,$50,$D0
HgrHiY:
        .byte $00,$00,$01,$01,$02,$02,$03,$03
        .byte $00,$00,$01,$01,$02,$02,$03,$03
        .byte $00,$00,$01,$01,$02,$02,$03,$03
SaveText80:	.res	1
.segment	"RODATA"
;.segment	"FONTDATA"
Font:
.include	"fontBB.inc"
;.segment	"TILEDATA"
Tile:
.include	"weatherEvenTile.inc"
;xFont:
;.include	"font78.inc"
.segment	"CODE"
