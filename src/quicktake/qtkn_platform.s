        .export       _consume_extra
        .export       _init_row
        .export       _decode_row

        .export       _init_top
        .export       _init_shiftl4
        .export       _init_next_line
        .export       _init_huff
        .export       tk1, tk2, tk3, tk4, got_4datahuff
        .export       discard_col_loop

        .import       _row_idx

        .import       get_4datahuff_interpolate, discard4datahuff_interpolate
        .import       _bitbuf_refill
        .import       _huff_data, _huff_ctrl, _src

        .import       huff_small_1
        .import       huff_small_2
        .import       huff_small_3
        .import       huff_small_4
        .import       huff_small_5

        .import       _factor, _last
        .import       _val_from_last, _val_hi_from_last
        .import       _next_line_l, _next_line_h
        .import       _div48
        .import       _dyndiv
        .import       _ushiftl3p4, _ushiftl4, _sshiftl4, _ushiftr4

        .import       mult16x16mid16_direct, mult8x8r16_direct
        .import       tosmula0, pushax, pusha0
        .import       _memset, _memcpy, aslax4
.ifdef APPROX_DIVISION
        .import       approx_div16x8_direct
.else
        .import       tosdiva0
.endif

        .importzp     _bitbuf, _vbits
        .importzp     tmp1, ptr2, tmp2, tmp3
        .importzp     _zp2, _zp3, _zp4, _zp6, _zp7, _zp8, _zp9, _zp10, _zp11

        .include      "qtkn_huffgetters.inc"
WIDTH               = 320
USEFUL_DATABUF_SIZE = 321
DATA_INIT           = 8

; ZP vars share locations when they can - they're usually limited to one function
val0             = _zp2   ; word - _decode_row
wordcnt          = _zp2   ; word - _init_divtable, _init_row
genptr           = _zp2   ; word - _init_huff

val1             = _zp4   ; word - _decode_row
src_idx          = _zp4   ; word - _init_huff

hval             = _zp6   ; word - _init_huff
col              = _zp6   ; byte - _decode_row, _consume_extra

rept             = _zp8   ; byte - _decode_row
tree             = _zp9  ; byte - _decode_row, _consume_extra
colh             = _zp10  ; byte - _decode_row, _consume_extra

                          ; _zp11-13 used by bitbuffer

.proc _init_shiftl4
        ldy     #$00

        ; shift left 4 (unsigned and signed)
:       ldx     #0
        tya
        bpl     :+
        dex
:       jsr     aslax4
        sta     _ushiftl4,y
        txa
        sta     _sshiftl4,y
        iny
        bne     :--

        ; shift right 4 (unsigned only)
:       tya
        lsr
        lsr
        lsr
        lsr
        sta     _ushiftr4,y
        iny
        bne     :-

        ; shift left 3 and add 4 (first 32 values)
        ldy     #31
:       tya
        asl
        asl
        asl
        ora     #$04
        sta     _ushiftl3p4,y
        dey
        bpl     :-

        rts
.endproc

.segment "LC"

.proc _init_next_line
        lda     #<_next_line_l
        ldx     #>_next_line_l
        jsr     pushax

        lda     #<2048
        jsr     pusha0

        lda     #<USEFUL_DATABUF_SIZE
        ldx     #>USEFUL_DATABUF_SIZE
        jsr     _memset

        lda     #<_next_line_h
        ldx     #>_next_line_h
        jsr     pushax

        lda     #>2048
        jsr     pusha0

        lda     #<USEFUL_DATABUF_SIZE
        ldx     #>USEFUL_DATABUF_SIZE
        jmp     _memset
.endproc

.proc _init_huff
        bit     $C083               ; WR-enable LC
        bit     $C083               ; we patch things.
        lda     #0
        sta     hval
        sta     hval+1
        sta     src_idx
        sta     src_idx+1
        sta     huff_ctrl_bits+1
        sta     huff_ctrl_value+1
        sta     huff_data_bits+1
        lda     #$80
        sta     huff_data_value+1

        ldx     #>_huff_ctrl
        stx     huff_ctrl_bits+2
        inx
        stx     huff_ctrl_value+2

        lda     #>_src
        sta     genptr+1
        lda     #<_src
        sta     genptr
ctrl_huff_next:
        ldy     src_idx

        lda     (genptr),y
        tax                         ; X = numbits

        stx     tmp1                ; 8-numbits
        lda     #8
        sec
        sbc     tmp1
        tay

        lda     hval                 ; code = hval & 0xFF
        cpy     #0
        beq     ctrl_code_shifted
:       lsr     a                   ; code >>= 8-numbits
        dey
        bne     :-

ctrl_code_shifted:
        sta     code
        txa
        tay
        lda     #$80                ; incr = 256 >> numbits
        dey
        beq     ctrl_incr_shifted
:       lsr     a
        dey
        bne     :-

ctrl_incr_shifted:
        sta     incr
        ldy     src_idx
        iny
        lda     (genptr),y          ; A = value

        ldy     code
huff_ctrl_value:
        sta     _huff_ctrl+256,y

        txa                         ; X = numbits
huff_ctrl_bits:
        sta     _huff_ctrl,y

        lda     src_idx             ; src_idx += 2
        clc
        adc     #2
        sta     src_idx
        bcc     :+
        inc     genptr+1

:       lda     hval                 ; hval += incr
        clc
        adc     incr
        sta     hval
        bcc     ctrl_huff_next

        lda     huff_ctrl_value+2
        clc
        adc     #2
        cmp     #>(_huff_ctrl+18*256)
        bcs     huff_ctrl_done
        sta     huff_ctrl_value+2
        inc     huff_ctrl_bits+2
        inc     huff_ctrl_bits+2
        jmp     ctrl_huff_next
huff_ctrl_done:

        ; Data huff now
        ldx     #>_huff_data
        stx     huff_data_bits+2
        stx     huff_data_value+2

data_huff_next:
        ldy     src_idx

        lda     (genptr),y
        tax                         ; X = numbits

        stx     tmp1                ; 8-numbits
        lda     #8
        sec
        sbc     tmp1
        tay

        lda     hval                 ; code = hval & 0xFF
        cpy     #0
        beq     data_code_shifted
:       lsr     a                   ; code >>= 8-numbits
        dey
        bne     :-

data_code_shifted:
        sta     code
        txa
        tay
        lda     #$80                ; incr = 256 >> numbits
        dey
        beq     data_incr_shifted
:       lsr     a
        dey
        bne     :-

data_incr_shifted:
        sta     incr
        ldy     src_idx
        iny
        lda     (genptr),y          ; A = value

        ldy     code
huff_data_value:
        sta     _huff_data+128,y

        txa                         ; X = numbits
huff_data_bits:
        sta     _huff_data,y

        lda     src_idx             ; src_idx += 2
        clc
        adc     #2
        sta     src_idx
        bcc     :+
        inc     genptr+1

:       lda     hval                 ; hval += incr
        clc
        adc     incr
        sta     hval
        bcc     data_huff_next

        lda     huff_data_value+2
        clc
        adc     #1
        cmp     #>(_huff_data+4*256)
        bcs     huff_data_done
        sta     huff_data_value+2
        inc     huff_data_bits+2
        jmp     data_huff_next
huff_data_done:
        rts
.endproc

.segment "CODE"

.proc _consume_extra
        lda     #3
        sta     pass

next_pass:
        dec     pass
        bne     repeat_loop
        rts

REFILLER ctrl_discard_fill, ctrl_discard_rts, #7

repeat_loop:
        lda     #1
        sta     tree
        lda     #<(WIDTH/2)
        sta     col

discard_col_loop:
        beq     next_pass     ; A = col

        lda     tree
        asl
        adc     #>_huff_ctrl
        sta     huff_numc
        adc     #1
        sta     huff_numc_h

        GETCTRLHUFF ctrl_discard_fill, ctrl_discard_rts
        sta     tree

        beq     data_repeat

data_standard:
        cmp     #8
        bne     data_interpolate

        DISCARD4DATAHUFF_INIT
        dec     col
        jmp     discard_col_loop

data_interpolate:
        jmp     discard4datahuff_interpolate ; will loop back to discard_col_loop

data_repeat:
        lda     col
        cmp     #2
        bcs     col_gt1

        ldx     #1
        jmp     check_nreps_2

REFILLER data0_refill, data0_rts, #7

col_gt1:
        GETDATAHUFF_NREPEATS data0_refill, data0_rts
        inx

check_nreps_2:
        cpx     #9
        bcc     store_nreps

        lda     col           ; nreps 9
        sec
        sbc     #8
        sta     col

        ldy     #4
        DISCARDNDATAHUFF_REPVAL
        jmp     data_repeat   ; nreps == 9 so keep going

store_nreps:
        stx     rep_loop_sub+1      ; nreps <= 8

        ; rep_loop /= 2
        txa
        lsr
        beq     skip_discard
        tay                   ; discard rep_loop/2 tokens
        DISCARDNDATAHUFF_REPVAL

skip_discard:
        ; col -= rep_loop
        lda     col
        sec
rep_loop_sub:
        sbc     #$FF
        sta     col
        jmp     discard_col_loop

        rts
.endproc

; Expects genptr to point to table
; factor in A
.proc _init_divtable
        sta     abck
        stx     xbck
        sty     ybck
        sta     fact+1

        lda     #0
        sta     wordcnt

:       ldx     wordcnt
        lda     #$80
.ifndef APPROX_DIVISION
        jsr     pushax
fact:   lda     #$FF
        jsr     tosdiva0
.else
fact:   ldy     #$FF
        jsr     approx_div16x8_direct
.endif
        ldy     wordcnt
        bmi     overflows_neg     ; stop if signed < 0
build_table_n:
        sta     $FF00,y
        txa
        bne     overflows         ; Stop if result > 256
        inc     wordcnt
        bne     :-
        jmp     done

overflows:                        ; Fill the rest
        lda     #$FF
build_table_o:
:       sta     $FF00,y
        iny
        bmi     overflows_neg
        bne     :-

overflows_neg:
        lda     #$00
build_table_u:
:       sta     $FF00,y
        iny
        bne     :-

done:
abck = *+1
        lda     #$FF
xbck = *+1
        ldx     #$FF
ybck = *+1
        ldy     #$FF

        rts
.endproc

.proc _init_top
        jsr     _init_huff
        jsr     _init_shiftl4
        ldx     #>_div48
        stx     _init_divtable::build_table_n+2
        stx     _init_divtable::build_table_o+2
        stx     _init_divtable::build_table_u+2

        lda     #48
        jsr     _init_divtable
        jmp     _init_next_line
.endproc

.macro INIT_BUF mult_factor, address, low_label, high_label
mult_factor:
        ldx     #$FF
        jsr     mult8x8r16_direct
        ldy     col
low_label: 
        sta     address,y
        txa
high_label: 
        sta     address,y
.endmacro

.macro INIT_VAL mult_factor, value, dest
        ldy     col
dest:   sta     $FFFF,y
mult_factor:
        ldx     #$FF
        jsr     mult8x8r16_direct
        sta     value
        stx     value+1
.endmacro


.macro INTERPOLATE_BUF_TOKEN val_a, addr2, l2, h2, val_b, addr_res, res_l, res_h, token
        clc
        lda     val_a
l2:     adc     addr2,y
        tax
        lda     val_a+1
h2:     adc     addr2,y
        cmp     #$80
        ror     a
        sta     tmp1
        txa
        ror     a

        clc
        adc     val_b
        tax
        lda     tmp1
        adc     val_b+1
        cmp     #$80
        ror     a

        .ifblank token
res_h:  sta     addr_res,y
        txa
        ror     a
res_l:  sta     addr_res,y
        .else
        sta     tmp1          ; High byte in tmp1
        txa
        ror     a             ; Low byte in A

token:  ldx     #$FF
        clc
        adc     _ushiftl4,x
res_l:  sta     addr_res,y
        lda     tmp1
        adc     _sshiftl4,x
res_h:  sta     addr_res,y
        .endif
.endmacro

.macro INTERPOLATE_VAL_TOKEN value, addr2, l2, h2, addr3, l3, h3, res, token, l4
        clc
        lda     value
l2:     adc     addr2,y
        tax
        lda     value+1
h2:     adc     addr2,y
        cmp     #$80
        ror     a
        sta     tmp1
        txa
        ror     a

        clc
l3:     adc     addr3,y
        sta     l4+1
        lda     tmp1
h3:     adc     addr3,y
        cmp     #$80
        ror     a

        .ifblank token
        sta     res+1
        tax                    ; Final high byte in X
l4:     lda     #$FF
        ror     a
        sta     res
        .else
        sta     tmp1          ; High byte in tmp1
l4:     lda     #$FF
        ror     a             ; Low byte in A

token:  ldx     #$FF
        clc
        adc     _ushiftl4,x
        sta     res
        lda     tmp1
        adc     _sshiftl4,x
        sta     res+1
        tax                  ; Final high byte in X
        .endif
.endmacro

.macro INCR_BUF_TOKEN addr1, l1, h1, addr2, l2, h2
        clc
        lda     _ushiftl4,x
l1:     adc     addr1,y
l2:     sta     addr2,y
        lda     _sshiftl4,x
h1:     adc     addr1,y
h2:     sta     addr2,y
.endmacro

.macro INCR_VAL_TOKEN value, divtable
        clc
        lda     _ushiftl4,x
        adc     value
        sta     value
        lda     _sshiftl4,x
        adc     value+1
        sta     value+1
        .ifblank divtable
        tax
        .else
        sta    divtable+1
        .endif
.endmacro

; Decode a pair of rows (but only keep the first one)
.proc _decode_row
        lda     #2            ; two passes to do
        sta     pass

        ldx     #0            ; Cheat and set col high byte to zero so 
        stx     colh          ; we drop into next_pass at first iteration

check_col_high:               ; Column loop high byte check
        ldx     colh          ; Out of main loop for performance
        bne     more_cols

next_pass:
        dec     pass          ; Is second pass done?
        bpl     :+
        jmp     all_passes_done

:       clc                   ; Advance row in output buffer
        lda     _row_idx
        adc     #<(WIDTH)
        sta     _row_idx
        tay                   ; For init_pass
        lda     _row_idx+1
        adc     #>(WIDTH)
        sta     _row_idx+1
        tax
        jsr     init_pass

decode_col_loop:
        ; 320 loop, repeated twice, function
        ; called 120 times: every line here
        ; costs 153-400ms depending on the
        ; number of cycles.

        lda     col               ; Is col loop done?
        beq     check_col_high

more_cols:

        ; Get "control" huff value from the last one (1 initially)
        lda     tree
        asl
        adc     #>_huff_ctrl        ; Patch the inlined decoder below
        sta     huff_numc
        adc     #1
        sta     huff_numc_h

        ; Get the new value
        GETCTRLHUFF rowctrl, rowctrlret

        sta     tree

        bne     data_standard       ; not 0 ? => "standard" data
        jmp     data_repeat         ; 0 ? => "repeat" data

; Out of main codepath, bitbuffer refiller for GETCTRLHUFF
REFILLER rowctrl, rowctrlret, #7

; Out of main codepath, pages decrementer when col reaches 256 (from 320)
dechigh:
        jsr     dec_buf_pages
        jmp     declow

; "Standard" data handler
data_standard:
        ldy     col
        beq     dechigh
declow:
        dey                         ; col -= 2
        dey
        sty     col

        lda     tree
        cmp     #DATA_INIT
        beq     data_init
        jmp     data_interpolate

; Out of main codepath refillers
REFILLER data9a_fill, data9a_rts, #7
REFILLER data9b_fill, data9b_rts, #7

; "init": Set values directly from 5 bits codes
data_init:
        GETDATAHUFF_INIT data9a_fill, data9a_rts
        ; val1 = code*factor
        INIT_VAL mult_factor1, val1, dest1a

        GETDATAHUFF_INIT data9b_fill, data9b_rts
        ; val0 = code*factor
        INIT_VAL mult_factor2, val0, dest0a

        GETDATAHUFF_INIT data9c_fill, data9c_rts
        ; next_line[col+2] = code*factor
        INIT_BUF mult_factor3, $FF02, next2la, next2ha

        GETDATAHUFF_INIT data9d_fill, data9d_rts
        ; next_line[col+1] = code*factor
        INIT_BUF mult_factor4, $FF01, next1la, next1ha

        ; Done for these two columns
        jmp     decode_col_loop

; Out of main codepath refillers
REFILLER data9c_fill, data9c_rts, #7
REFILLER data9d_fill, data9d_rts, #7

; "interpolate": Get values from data Huffman tables, do average them with surrounding pixels
data_interpolate:
        asl                   ; A is "tree" aka the current Huffman table to use
        sta     jump+1

jump:   ; Will get four tokens and store them in tk1-4, in code, below
        jmp     (get_4datahuff_interpolate)

got_4datahuff:
        ldy     col           ; Reload col=Y, it got destroyed by Huffman decoders

        ; Mind that the order counts, as next_line[col+2] depends on the previous loop's val0.

        ; val1 = ((((val0 + next_line[col+2]) >> 1) + next_line[col+1]) >> 1) + tk1;
        INTERPOLATE_VAL_TOKEN val0, $FF02, next2lb, next2hb, $FF01, next1lb, next1hb, val1, tk1, l4a
divt1b: lda     _div48,x    ; Store to output buffer
dest1b: sta     $FFFF,y

        ; next_line[col+2] = ((((val0 + next_line[col+3]) >> 1) + val1) >> 1) + tk3;
        INTERPOLATE_BUF_TOKEN val0, $FF03, next3la, next3ha, val1, $FF02, next2lc, next2hc, tk3

        ; val0 = ((((val1 + next_line[col+1]) >> 1) + next_line[col+0]) >> 1) + tk2;
        INTERPOLATE_VAL_TOKEN val1, $FF01, next1lc, next1hc, $FF00, next0la, next0ha, val0, tk2, l4b
divt0b: lda     _div48,x    ; Store to output buffer
dest0b: sta     $FFFF,y

        ; next_line[col+1] = ((((val1 + next_line[col+2]) >> 1) + val0) >> 1) + tk4;
        INTERPOLATE_BUF_TOKEN val1, $FF02, next2ld, next2hd, val0, $FF01, next1ld, next1hd, tk4

        ; Done for this pair of columns
        jmp     decode_col_loop

; "Repeat" data. 
data_repeat:
nine_reps_loop:
        ldx     colh          ; Check whether col > 2
        bne     col_gt2
        lda     col
        cmp     #3
        bcs     col_gt2
        ldx     #1            ; If not, 1 repeat only
        jmp     check_nreps

; Out of main codepath decrementer
dechigh2:
        jsr     dec_buf_pages
        jmp     declow2

; Out of main codepath refiller for Huffman decoder
REFILLER data0_refill, data0_rts, #7
col_gt2:
        ; col > 2, get number of repeats from data
        GETDATAHUFF_NREPEATS data0_refill,data0_rts

        inx                   ; and add one.
check_nreps:
        lda     #$2C          ; BIT - prepare end-of-loop patching
        cpx     #9            ; if repeats >= 9, we'll loop again until it's not
        bcc     nreps_check_done
        lda     #$4C          ; JMP
        ldx     #8            ; but we'll repeat 8 times, not 9.
nreps_check_done:
        stx     rep_loop_check+1
        sta     rep_loop_done ; Patch end of loop to continue if nreps>=9

        ldx     #$00
do_rep_loop:
        stx     rept

        ldy     col
        beq     dechigh2

declow2:
        dey                   ; col -= 2
        dey
        sty     col

        ; Same as earlier, order is important.
        ; val1 = ((((val0 + next_line[col+2]) >> 1) + next_line[col+1]) >> 1);
        INTERPOLATE_VAL_TOKEN val0, $FF02, next2le, next2he, $FF01, next1le, next1he, val1, , l4c
divt1c: lda     _div48,x    ; Store to output buffer while we're at it.
dest1c: sta     $FFFF,y       ; Annoyingly this is 8 cycles lost if repeat is odd

        ; next_line[col+2] = ((((val0 + next_line[col+3]) >> 1) + val1) >> 1);
        INTERPOLATE_BUF_TOKEN val0, $FF03, next3lb, next3hb, val1, $FF02, next2lf, next2hf

        ; val0 = ((((val1 + next_line[col+1]) >> 1) + next_line[col+0]) >> 1);
        INTERPOLATE_VAL_TOKEN val1, $FF01, next1lf, next1hf, $FF00, next0lb, next0hb, val0, , l4d
divt0c: lda     _div48,x    ; Same. It's better to lose these 16 cycles than to
dest0c: sta     $FFFF,y       ; test for evenness, for now.

        ; next_line[col+1] = ((((val1 + next_line[col+2]) >> 1) + val0) >> 1);
        INTERPOLATE_BUF_TOKEN val1, $FF02, next2lg, next2hg, val0, $FF01, next1lg, next1hg

        lda     rept          ; rep & 1 ? if rep is odd,
        and     #1
        beq     rep_even

        ; tk = gethuffdata(1) << 4;
        GETDATAHUFF_REPVAL repval_refill, repval_rts   ; we patch the values we just computed with this token.
        ; tk in X now

        ldy     col           ; Reload col...

        ; Increment next_line values by token (in X)
        INCR_BUF_TOKEN $FF02, next2lh, next2hh, $FF02, next2li, next2hi
        INCR_BUF_TOKEN $FF01, next1lh, next1hh, $FF01, next1li, next1hi

        ; Increment values by token (in X) - preserve X on the first one
        INCR_VAL_TOKEN val1, divt1d
divt1d: lda     _div48      ; Store the updated values into output buffer
dest1d: sta     $FFFF,y

        INCR_VAL_TOKEN val0
divt0d: lda     _div48,x
dest0d: sta     $FFFF,y


rep_even:
        ldx     rept          ; rep++
        inx
rep_loop_check:
        cpx     #$FF
        beq     rep_loop_done
        jmp     do_rep_loop

REFILLER repval_refill, repval_rts, #7, store

rep_loop_done:
        jmp     nine_reps_loop; Patched with bit/jmp depending on whether nreps >= 9

        jmp     decode_col_loop

all_passes_done:

        rts
.endproc

.proc _init_row
        ldx     #>(_next_line_l+256)      ; start at second page
        stx     load_next_lf+2
        stx     load_next_ls+2
        stx     store_next_lf+2
        stx     store_next_ls+2
        ldx     #>(_next_line_h+256)
        stx     load_next_hf+2
        stx     load_next_hs+2
        stx     store_next_hf+2
        stx     store_next_hs+2

        jsr     init_factor

        ldy     _last               ; is last 8bits?
        cpy     #18
        bcs     small_val

        lda     _val_from_last,y    ; no, do a 16x8 mult
        ldx     _val_hi_from_last,y
        jsr     pushax
        lda     _factor
        sta     _last
        jsr     tosmula0
        jmp     check_multiplier

small_val:                          ; Last is 8bit, do a small mult
        ldy     _last               ; Reload last
        ldx     _val_from_last,y    ; and multiply
        lda     _factor
        sta     _last
        jsr     mult8x8r16_direct

check_multiplier:
        cpx     #$0F                ; is multiplier 0xFF?
        bne     check_0x100         ; Check before shifting it
        cmp     #$F0                ; as ((tmp*0xFF) >> 8) is also ((tmp<<8 - tmp) >> 8)
        bcc     check_0x100         ; and in turn (tmp - (tmp>>8)), which is much faster.

mult_0xFF:                          ; It is!
        ldx     #<USEFUL_DATABUF_SIZE
        lda     #>USEFUL_DATABUF_SIZE
        sta     wordcnt             ; Number of pages

setup_curbuf_x_ff:
        dex

load_next_lf:
        lda     $FF00,x             ; Low byte of next_line[x] in A,
        sec
load_next_hf:
        sbc     $FF00,x             ; Subtract high byte from low : -(tmp>>8)
        bcs     store_next_lf
store_next_hf:
        dec     $FF00,x             ; Decrement high byte if needed

store_next_lf:
        sta     $FF00,x             ; Update low byte

        cpx     #0
        bne     setup_curbuf_x_ff
        dec     load_next_hf+2
        dec     load_next_lf+2
        dec     store_next_hf+2
        dec     store_next_lf+2
        dec     wordcnt
        bpl     setup_curbuf_x_ff
        rts

check_0x100:
        cpx     #10                 ; or 0x100? in which case we have nothing to do
        bne     :+                  ; as ((tmp*0x100) >> 8) = ((tmp<<8)>>8) = tmp
        cmp     #10
        bcc     init_done

:       tay                         ; No luck. Arbitrary multiplier, shift it >> 4
        lda     _ushiftr4,y
        ora     _ushiftl4,x
        sta     ptr2
        lda     _ushiftr4,x
        sta     ptr2+1              ; multiplier stored in ptr2/+1

slow_mults:                         ; and multiply next_line[], slowly
        ldy     #<USEFUL_DATABUF_SIZE
        lda     #>USEFUL_DATABUF_SIZE
        sta     wordcnt+1

setup_curbuf_x_slow:
        ; load
        dey
        sty     wordcnt
load_next_hs:
        ldx     $FF00,y
load_next_ls:
        lda     $FF00,y

        cpx     #$80
        bcc     posmult
        ; reverse sign, less expensive than extending
        ; and doing a 16x24 mult, especially since it rarely happens,
        ; only on badly underexposed pictures apparently
        eor     #$FF
        adc     #0            ; Carry set by cpx already
        tay
        txa
        eor     #$FF
        adc     #0
        tax
        tya
        ; multiply
        jsr     mult16x16mid16_direct
        clc
        adc     #1            ; And reverse sign back
        clc
        eor     #$FF
        adc     #1
        tay
        txa
        eor     #$FF
        adc     #0
        tax
        tya
        jmp     set0
posmult:
        jsr     mult16x16mid16_direct

set0:
        ldy     wordcnt
store_next_ls:
        sta     $FF00,y
        txa
store_next_hs:
        sta     $FF00,y

        cpy     #0
        bne     setup_curbuf_x_slow
        dec     load_next_hs+2
        dec     load_next_ls+2
        dec     store_next_hs+2
        dec     store_next_ls+2
        dec     wordcnt+1
        bpl     setup_curbuf_x_slow
init_done:
        rts
.endproc

.segment "LC"

.proc init_pass
        ; Set the output buffer pointers (even columns)
        ; Low byte has to be set as we move in the
        ; output buffer by 320 at a time, it won't
        ; stay zero.
        sty     _decode_row::dest0a+1
        sty     _decode_row::dest0b+1
        sty     _decode_row::dest0c+1
        sty     _decode_row::dest0d+1
        inx                       ; Start at page 2
        stx     _decode_row::dest0a+2
        stx     _decode_row::dest0b+2
        stx     _decode_row::dest0c+2
        stx     _decode_row::dest0d+2

        ; and odd columns
        iny
        bne     :+
        inx
:       sty     _decode_row::dest1a+1
        sty     _decode_row::dest1b+1
        sty     _decode_row::dest1c+1
        sty     _decode_row::dest1d+1
        stx     _decode_row::dest1a+2
        stx     _decode_row::dest1b+2
        stx     _decode_row::dest1c+2
        stx     _decode_row::dest1d+2

        ; factor<<7: 00000000 ABCDEFGH becomes 0ABCDEFG H0000000
        lda     _factor
        lsr     a                   ; shift low byte >>1, (lose low bit to carry),
        tax                         ; move what remains to high byte (0ABCDEFG)
        lda     #0                  ; reinit low byte to zero,
        ror     a                   ; put initial low bit back to high bit of low byte

        ; val0 = next_line[WIDTH+1] = factor<<7
        stx     _next_line_h+(WIDTH+1)
        stx     val0+1
        sta     _next_line_l+(WIDTH+1)
        sta     val0

        ; tree = 1,
        lda     #1
        sta     tree

        ; col = WIDTH
        lda     #<(WIDTH)
        sta     col
        ldx     #>(WIDTH)
        stx     colh

        ; Init the numerous patched locations
        ; Worth the ugliness as there are 54
        ; locations, and we run that path
        ; 320*120*2 times so doing $nnnn,y
        ; instead of ($nn), y we spare:
        ; (320*120*2)*(2*54): 7 SECONDS
        ; (and spend only 0.165s shifting
        ; the high bytes, only 120*2 times)
        ldx     #>(_next_line_h+256)
        lda     #>(_next_line_l+256)
        jmp     set_buf_pages
.endproc

.proc dec_buf_pages
        ; Decrement output buffer pointers by one page,
        dec     colh
        dec     _decode_row::dest0a+2
        dec     _decode_row::dest0b+2
        dec     _decode_row::dest0c+2
        dec     _decode_row::dest0d+2
        dec     _decode_row::dest1a+2
        dec     _decode_row::dest1b+2
        dec     _decode_row::dest1c+2
        dec     _decode_row::dest1d+2

        ; Load next_line[] first pages addresses,
        ldx     #>(_next_line_h)
        lda     #>(_next_line_l)
        ; Fallthrough to set_buf_pages
.endproc

.proc set_buf_pages
        ; Store it everywhere
        stx     _decode_row::next0ha+2
        stx     _decode_row::next0hb+2

        stx     _decode_row::next1ha+2
        stx     _decode_row::next1hb+2
        stx     _decode_row::next1hc+2
        stx     _decode_row::next1hd+2
        stx     _decode_row::next1he+2
        stx     _decode_row::next1hf+2
        stx     _decode_row::next1hg+2
        stx     _decode_row::next1hh+2
        stx     _decode_row::next1hi+2

        stx     _decode_row::next2ha+2
        stx     _decode_row::next2hb+2
        stx     _decode_row::next2hc+2
        stx     _decode_row::next2hd+2
        stx     _decode_row::next2he+2
        stx     _decode_row::next2hf+2
        stx     _decode_row::next2hg+2
        stx     _decode_row::next2hh+2
        stx     _decode_row::next2hi+2

        stx     _decode_row::next3ha+2
        stx     _decode_row::next3hb+2

        sta     _decode_row::next0la+2
        sta     _decode_row::next0lb+2

        sta     _decode_row::next1la+2
        sta     _decode_row::next1lb+2
        sta     _decode_row::next1lc+2
        sta     _decode_row::next1ld+2
        sta     _decode_row::next1le+2
        sta     _decode_row::next1lf+2
        sta     _decode_row::next1lg+2
        sta     _decode_row::next1lh+2
        sta     _decode_row::next1li+2

        sta     _decode_row::next2la+2
        sta     _decode_row::next2lb+2
        sta     _decode_row::next2lc+2
        sta     _decode_row::next2ld+2
        sta     _decode_row::next2le+2
        sta     _decode_row::next2lf+2
        sta     _decode_row::next2lg+2
        sta     _decode_row::next2lh+2
        sta     _decode_row::next2li+2

        sta     _decode_row::next3la+2
        sta     _decode_row::next3lb+2
        rts
.endproc

.proc init_factor
        ; Hardcode the factor for next loop, faster to spend 16 cycles there
        ; and avoid 320*4*2 cycles reloading it on each column
        lda     _factor

        sta     _decode_row::mult_factor1+1
        sta     _decode_row::mult_factor2+1
        sta     _decode_row::mult_factor3+1
        sta     _decode_row::mult_factor4+1

        cmp     #48
        beq     set_div48

set_dyndiv:
        ldx     #>_dyndiv         ; Factor not 48, update division table pointers
        cmp     last_dyndiv         ; Factor different than last one,
        beq     factor_done
        stx     _init_divtable::build_table_n+2
        stx     _init_divtable::build_table_o+2
        stx     _init_divtable::build_table_u+2
        sta     last_dyndiv
        jsr     _init_divtable      ; Rebuild current factor division table
        ldx     #>_dyndiv
        jmp     factor_done

set_div48:
        ldx     #>_div48            ; Set /48 division table

factor_done:
        stx     _decode_row::divt0b+2
        stx     _decode_row::divt0c+2
        stx     _decode_row::divt0d+2
        stx     _decode_row::divt1b+2
        stx     _decode_row::divt1c+2
        stx     _decode_row::divt1d+2

        rts
.endproc
; Scoped exports for bitbuffer
tk1           = _decode_row::tk1
tk2           = _decode_row::tk2
tk3           = _decode_row::tk3
tk4           = _decode_row::tk4
got_4datahuff = _decode_row::got_4datahuff
discard_col_loop = _consume_extra::discard_col_loop

.segment "BSS"

pass:           .res 1
incr:           .res 1
code:           .res 1
last_dyndiv:    .res 1
