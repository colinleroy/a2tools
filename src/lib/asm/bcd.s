        .export   do_bin2dec_16bit, do_bin2dec_8bit
        .export   bcd_result, bcd_result_thousand

; Supports binary to decimal conversion up to 8 digits.
; See readme.txt for documentation.

;Copyright (c) 2006 Quietust
;Copyright (c) 2006 by Shay Green. Permission is hereby granted, free of charge,
;to any person obtaining a copy of this software and associated documentation
;files (the "Software"), to deal in the Software without restriction, including
;without limitation the rights to use, copy, modify, merge, publish, distribute,
;sublicense, and/or sell copies of the Software, and to permit persons to whom
;the Software is furnished to do so, subject to the following conditions: The
;above copyright notice and this permission notice shall be included in all
;copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS
;IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
;LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
;AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
;LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
;CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
;SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

vtable: .byte 3,6,0,1,2,5,0,1,3,6,0

bdgood: .byte 5,5,4,4,4,4,3,3,3,3,2,2,2,2,1,1,1,1 ; last entry is 0, which is in b2table

; The following tables use some portion of these values:
;                      400,    700,
;     1000,   2000,   3000,   6000,
;    10000,  20000,  40000,  70000,
;   100000, 200000, 300000, 600000,
;  1000000,2000000,4000000,7000000,
; 10000000

b2table: ; high bytes of one less than 40,000 ... 10,000,000
        .byte                                 $00,$01,$01,$03,$04,$09,$0F,$1E,$3D,$6A,$98

b1table: ; middle bytes of one less than 400 ... 10,000,000
        .byte $01,$02,$03,$07,$0B,$17,$27,$4E,$9C,$11,$86,$0D,$93,$27,$42,$84,$08,$CF,$96

b0table: ; low bytes of one less than 400 ... 10,000,000
        .byte $8F,$BB,$E7,$CF,$B7,$6F,$0F,$1F,$3F,$6F,$9F,$3F,$DF,$BF,$3F,$7F,$FF,$BF,$7F

do_bin2dec_16bit:
        sta     binary
        stx     binary+1
        lda     #0
        jsr     bin2dec_init
        jmp     bin2dec_16bit

do_bin2dec_8bit:
        sta     binary
        lda     #0
        sta     binary+1
        jsr     bin2dec_init
        jmp     bin2dec_8bit

bin2dec_init:                   ; A = value of zero digit
        ldy    #7
bin2dec_init_loop:
        sta    bcd_result,Y
        dey
        bpl    bin2dec_init_loop
        rts

bin2dec_24bit:
        ldy    #10
        clc
        bcc    bin2dec_24bit_custom
        
bin2dec_16bit:
        ldy    #8
        clc
        bcc    bin2dec_16bit_custom
        
good24: 
        sta     binary+2
        stx     binary+1
        
        ldx     bdgood+8,Y
        lda     bcd_result,X
        adc     vtable,Y
        sta     bcd_result,X
        
        lda     binary+0
        sbc     b0table+8,Y
        sta     binary+0
        clc
        dey
        bmi     end24
        beq     end24               ; was 70000 or greater, so skip 40000 check
        
bin2dec_24bit_custom:           ; 8 digits: Y=10, 7: Y=9, 6: Y=5, 5: Y=1
loop24: 
        lda     binary+0
        sbc     b0table+8,Y
        lda     binary+1
        sbc     b1table+8,Y
        tax
        lda     binary+2
        beq     bin2dec_16bit       ; optimization only - can be removed
        sbc     b2table,Y
        bcs     good24
        dey
        bpl     loop24
end24:
        ldy     #7
        bne     loop16              ; always branches
        
good16:
        stx     binary+0
        sta     binary+1
        ldx     bdgood,Y
        lda     bcd_result,X
        adc     vtable,Y
        sta     bcd_result,X
        dey
        bmi     end16
        beq     end16

bin2dec_16bit_custom:           ; 5 digits: Y=8, 4: Y=5, 3: Y=1
loop16:
        lda     binary+0
        sbc     b0table,Y
        tax
        lda     binary+1
        beq     bin2dec_8bit        ; optimization only - can be removed
        sbc     b1table,Y
        bcs     good16
        dey
        bpl     loop16
end16:  lda     binary+0
        sbc     #199                ; +1 for clear carry
        ldx     binary+1
        bne     gt200               ; high byte non-zero, so must be >= 200
        
bin2dec_8bit:                   ; 3 digits
        lda     binary+0
bin2dec_8bit_3dig:              ; A to 3 digits
cp200:
        cmp     #200
        bcc     lt200
        sbc     #200
gt200:
        inc     bcd_result+5
        inc     bcd_result+5
lt200:
        cmp     #100
        bcc     lt100
        sbc     #100
        inc     bcd_result+5
bin2dec_8bit_2dig:              ; A to 2 digits
lt100:
        ldx     #0
        cmp     #70
        bcc     lt70
        sbc     #70
        ldx     #7
lt70:
        cmp     #40
        bcc     lt40
        sbc     #40
        ldx     #4
lt40:
        cmp     #20
        bcc     lt20
        sbc     #20
        inx
        inx
lt20:
        cmp     #10
        bcc     lt10
        sbc     #10
        inx
        clc
lt10:
        adc     bcd_result+7
        sta     bcd_result+7
        txa
        adc     bcd_result+6
        sta     bcd_result+6
        rts

  .bss
binary:     .res 2
bcd_result: .res 8
bcd_result_thousand = bcd_result+5
