        .importzp        sp, sreg
        .importzp        tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4
        .importzp        _prev_rom_irq_vector, _zp6p, _zp8p, _zp10p, _zp12, _zp13, _zp12ip

        .import          _memcpy, _memset, _progress_bar
				.import          pushax, pusha, pusha0, decsp6, incsp6, subysp
        .import          _height
        .import          _width
        .import          _raw_image
        .import          _reset_bitbuff
        .import          _get_four_bits

        .export          _got_four_bits
        .export          _magic
        .export          _model
        .export          _huff_ptr
        .export          _qt_load_raw
        .export          _cache
        .export          _cache_start

; Defines

BAND_HEIGHT   = 20
SCRATCH_WIDTH = (640 + 4)
SCRATCH_HEIGHT= (BAND_HEIGHT + 4)
PIXELBUF_SIZE = (SCRATCH_HEIGHT * SCRATCH_WIDTH + 2)

.segment        "DATA"

_magic:
        .byte        $71,$6B,$74,$6B,$00
_model:
        .addr        model_str
_huff_ptr:
        .word        $0000
_cache_start:
        .addr        _cache

.segment        "RODATA"

gstep_low:
        .byte        $A7
        .byte        $C4
        .byte        $D4
        .byte        $E0
        .byte        $EA
        .byte        $F1
        .byte        $F8
        .byte        $FE

        .byte        $02
        .byte        $08
        .byte        $0F
        .byte        $16
        .byte        $20
        .byte        $2C
        .byte        $3C
        .byte        $59

gstep_high:
        .byte        $FF
        .byte        $FF
        .byte        $FF
        .byte        $FF
        .byte        $FF
        .byte        $FF
        .byte        $FF
        .byte        $FF

        .byte        $00
        .byte        $00
        .byte        $00
        .byte        $00
        .byte        $00
        .byte        $00
        .byte        $00
        .byte        $00

model_str:
        .byte        $31,$30,$30,$00

.segment        "BSS"

_cache:
        .res        CACHE_SIZE,$00
dst:
        .res        2,$00
width_plus2:
        .res        2,$00
pgbar_state:
        .res        2,$00
last_two_lines:
        .res        2,$00
third_line:
        .res        2,$00
at_very_first_row:
        .res        1,$00
pixelbuf:
        .res        PIXELBUF_SIZE,$00
pix_direct_row:
        .res        (2*(SCRATCH_HEIGHT)),$00

; ---------------------------------------------------------------
; void __near__ qt_load_raw (unsigned int top)
; ---------------------------------------------------------------

.segment        "CODE"

val               = _prev_rom_irq_vector
row               = _prev_rom_irq_vector+1

src               = _zp6p
idx               = _zp8p
idx_behind        = _zp10p

; idx_pix_rows is used before at_very_first_col and last_val,
; so they share the same ZP address
idx_pix_rows      = _zp12ip
at_very_first_col = _zp12
last_val          = _zp13

.segment        "CODE"
_qt_load_raw:
        cmp     #$00
        bne     :+
        cpx     #$00
        beq     top
:       jmp     not_top

top:    jsr     _reset_bitbuff  ; Yes. Initialize things

        lda     _width
        clc
        adc     #2
        sta     width_plus2
        lda     _width+1
        adc     #0
        sta     width_plus2+1

        stz     pgbar_state

        lda     #<(pixelbuf)
        sta     idx
        lda     #>(pixelbuf)
        sta     idx+1

        lda     #<(pix_direct_row)
        sta     idx_pix_rows
        lda     #>(pix_direct_row)
        sta     idx_pix_rows+1

        ldx     #(SCRATCH_HEIGHT)
        ldy     #1
        sty     at_very_first_row
precalc_row_index:              ; Init direct pointers to each line
        lda     idx
        sta     (idx_pix_rows)
        lda     idx+1
        sta     (idx_pix_rows),y

        lda     #<SCRATCH_WIDTH
        adc     idx
        sta     idx
        lda     #>SCRATCH_WIDTH
        adc     idx+1
        sta     idx+1

        lda     idx_pix_rows
        adc     #2
        sta     idx_pix_rows
        bcc     :+
        inc     idx_pix_rows+1
        clc

:       dex
        bne     precalc_row_index

        ; Calculate offset to raw start of line 20
        lda     pix_direct_row+(BAND_HEIGHT*2)
        sta     last_two_lines
        lda     pix_direct_row+(BAND_HEIGHT*2)+1
        sta     last_two_lines+1

        ; Calculate offset to final start of third line, line 2
        lda     pix_direct_row+(2*2)
        adc     #2
        sta     third_line
        lda     pix_direct_row+(2*2)+1
        adc     #0
        sta     third_line+1

        ; Fill whole buffer with grey
        lda     #<(pixelbuf)
        ldx     #>(pixelbuf)
        jsr     pushax
        lda     #$80
        jsr     pusha0
        lda     #<PIXELBUF_SIZE
        ldx     #>PIXELBUF_SIZE
        jsr     _memset

        jmp     start_work
not_top:
        ; Shift the previous band's last two lines, plus 2 pixels,
        ; to the start of the new band.
        lda     #<(pixelbuf)
        ldx     #>(pixelbuf)
        jsr     pushax
        lda     last_two_lines
        ldx     last_two_lines+1
        jsr     pushax
        lda     #<(2*SCRATCH_WIDTH + 2)
        ldx     #>(2*SCRATCH_WIDTH + 2)
        jsr     _memcpy

        ; Reset the rest of the lines with grey
        lda     third_line
        ldx     third_line+1
        jsr     pushax
        lda     #$80
        jsr     pusha0
        lda     #<(PIXELBUF_SIZE-(2*SCRATCH_WIDTH + 2))
        ldx     #>(PIXELBUF_SIZE-(2*SCRATCH_WIDTH + 2))
        jsr     _memset

start_work:
        ; We start at line 2
        lda     pix_direct_row+(2*2)+1
        sta     src+1
        lda     pix_direct_row+(2*2)
        sta     src

        ; We iterate over 20 lines
        lda     #BAND_HEIGHT
        sta     row

first_pass_next_row:
        lda     row             ; Row & 1?
        bit     #$01
        beq     even_row

        clc
        lda     src             ; idx_end = src + width_plus2 + 1
        adc     width_plus2
        tay
        lda     src+1
        adc     width_plus2+1
        iny                     ; + 1
        sty     check_first_pass_col_loop+1
        bne     :+
        inc     a
:       sta     check_first_pass_col_loop_hi+1

        lda     src             ; Set idx_forward = src + SCRATCH_WIDTH and idx = src + 1
        tay
        adc     #<SCRATCH_WIDTH
        sta     store_idx_forward+1
        lda     src+1
        tax
        adc     #>SCRATCH_WIDTH
        sta     store_idx_forward+2

        iny                     ; Finish with idx = src + 1
        bne     :+
        inx

:       sty     idx
        stx     idx+1

        jmp     first_pass_row_work
even_row:
        and     #$02            ; Row % 8?
        bne     :+
        jsr     update_progress_bar

:       clc
        lda     src             ; idx_end = src + width_plus2
        adc     width_plus2
        sta     check_first_pass_col_loop+1
        lda     src+1
        adc     width_plus2+1
        sta     check_first_pass_col_loop_hi+1

        lda     src             ; Set idx_forward = src + SCRATCH_WIDTH + 1 and idx = src
        sta     idx
        adc     #<(SCRATCH_WIDTH+1)
        sta     store_idx_forward+1
        lda     src+1
        sta     idx+1
        adc     #>(SCRATCH_WIDTH+1)
        sta     store_idx_forward+2

first_pass_row_work:
        lda     (idx)           ; Remember previous val before shifting
        sta     last_val        ; index

        lda     idx
        ldx     idx+1
        sta     store_idx_min2+1; Remember idx-2 for first columns
        stx     store_idx_min2+2

        clc
        adc     #2
        bcc     :+
        inx

:       sta     idx             ; idx += 2
        stx     idx+1

        sec                     ; Set idx_behind = idx - (SCRATCH_WIDTH+1)
        sbc     #<(SCRATCH_WIDTH+1)
        sta     idx_behind
        txa
        sbc     #>(SCRATCH_WIDTH+1)
        sta     idx_behind+1

        clc
        lda     #<SCRATCH_WIDTH ; src += SCRATCH_WIDTH
        adc     src
        sta     src
        lda     #>SCRATCH_WIDTH
        adc     src+1
        sta     src+1

        ; We're at first column
        sta     at_very_first_col

first_pass_col_loop:
        jmp     _get_four_bits
_got_four_bits:

        ; Thanks to Kent Dickey for the simplification!
        ; val = ((*idx_behind            // row-1, col-1
        ;      + last_val) >> 1    // row,   col-2
        ;      + *idx_behind+2) >> 1 // row-1, col+1
        ;      + gstep[h];

        clc
        lda     (idx_behind)
        adc     last_val
        ror

        clc
        ldy     #2
        adc     (idx_behind),y
        ror

        clc                     ; + gstep[h].
        adc     gstep_low,x     ; Sets carry if overflow

        sta     (idx)           ; *idx = val
        tay                     ; Backup val's low byte to Y for later

        lda     gstep_high,x    ; Carry set by previous adc if overflowed
        adc     #0
        beq     val_stored      ; No overflow
        asl     a               ; Get sign (sets carry if negative)
        lda     #$FF
        adc     #0              ; FF => 00 if carry set

store_val:
        sta     (idx)           ; *idx = val
        tay

val_stored:
        sty     last_val        ; last_val = val

        clc                     ; idx_behind += 2
        lda     idx_behind
        adc     #2
        sta     idx_behind
        bcc     :+
        inc     idx_behind+1

:       ldx     at_very_first_col
        bne     handle_first_col

not_at_first_col:
        ldx     at_very_first_row
        bne     handle_first_row

not_at_first_row:
        clc                     ; idx += 2
        lda     idx
        adc     #2
        sta     idx
        bcc     check_first_pass_col_loop
        inc     idx+1

check_first_pass_col_loop:
        cmp     #0              ; Patched (idx_end)
        bne     first_pass_col_loop
        lda     idx+1
check_first_pass_col_loop_hi:
        cmp     #0              ; Patched (idx_end+1)
        bne     first_pass_col_loop

end_of_line:
        tya                     ; *idx = val (still in Y)
        sta     (idx)
        stz     at_very_first_row

        dec     row
        beq     start_second_pass
        jmp     first_pass_next_row


        ; First cols and first row handlers, out of main loop

handle_first_col:               ; *(idx_forward) = *(idx_min2) = val (still in Y)
store_idx_forward:
        sty     $FFFF           ; Patched
store_idx_min2:
        sty     $FFFF           ; Patched
        stz     at_very_first_col
        jmp     not_at_first_col; Back to common codepath

handle_first_row:               ; *(idx_behind+2) = *(idx_behind) = val
        tya                     ; get val back from Y
        ldy     #2
        sta     (idx_behind),y
        sta     (idx_behind)
        tay                     ; set Y back to val
        jmp     not_at_first_row

        ; End of first col/row handlers


start_second_pass:
        lda     pix_direct_row+(2*2)+1
        sta     src+1
        lda     pix_direct_row+(2*2)
        sta     src

        lda     #BAND_HEIGHT
        sta     row

second_pass_next_row:
        clc
        ldy     src
        ldx     src+1

        iny
        bne     :+
        inx

:       lda     row             ; row & 1?
        bit     #$01
        bne     second_pass_row_work
        iny                     ; no,  idx = src + 2
        bne     second_pass_row_work
        inx

second_pass_row_work:
        sty     idx
        stx     idx+1

        tya
        adc     _width          ; idx_end = idx + width
        sta     check_second_pass_col_loop+1
        txa
        adc     _width+1
        sta     check_second_pass_col_loop_hi+1

        lda     #<SCRATCH_WIDTH ; src += SCRATCH_WIDTH
        adc     src
        sta     src
        lda     #>SCRATCH_WIDTH
        adc     src+1
        sta     src+1

second_pass_col_loop:
        ; val = (*(idx+1) << 1)
        ;    + ((*(idx) + *(idx+2)) >> 1)
        ;    - 0x100;

        ldx     #1
        ldy     #2
        clc
        lda     (idx)
        adc     (idx),y
        ror                     ; >> 1 and get carry back to high bit
        sta     tmp1
        dey
        lda     (idx),y  ; *idx << 1
        asl

        bcc     :+
        dex
        clc

:       adc     tmp1
        bcc     :+
        dex

        ; Now X = 1 means < 0, X = 0 means val in range, X = $FF means > 255
:       cpx     #0
        beq     :+
        txa
        bmi     :+              ; $FF is good as is
        dec     a               ; Transform 1 to 0

:       sta     (idx),y  ; *(idx+1) = val (Y still 1)

        ; Shift indexes by 2, in order

        ldx     idx+1
        lda     idx
        clc
        adc     #2
        sta     idx
        bcc     check_second_pass_col_loop
        inx
        stx     idx+1

check_second_pass_col_loop:
        ; Are we done for this row?
        cmp     #0              ; Patched (idx_end)
        bne     second_pass_col_loop
check_second_pass_col_loop_hi:
        cpx     #0              ; Patched (idx_end+1)
        bne     second_pass_col_loop

second_pass_row_done:
        dec     row
        bne     second_pass_next_row

        ; Both passes done, memcpy BAND_HEIGHT lines to destination buffer,
        ; excluding two leftmost and rightmost scratch pixels 
        lda     #<(_raw_image)
        sta     dst
        ldx     #>(_raw_image)
        stx     dst+1
        jsr     pushax

        clc
        lda     pix_direct_row+(2*2)
        ldx     pix_direct_row+(2*2)+1
        adc     #2
        sta     src
        bcc     :+
        inx
        clc
:       stx     src+1
        jsr     pushax

        lda     #BAND_HEIGHT
        sta     row

copy_row:
        lda     _width
        ldx     _width+1
        jsr     _memcpy

        dec     row
        beq     copy_done

        clc                     ; dst += width
        lda     dst
        adc     _width
        sta     dst
        tay
        lda     dst+1
        adc     _width+1
        sta     dst+1
        tax
        tya
        jsr     pushax          ; push dst to memcpy

        clc                     ; src += SCRATCH_WIDTH
        lda     src
        adc     #<SCRATCH_WIDTH
        sta     src
        tay
        lda     src+1
        adc     #>SCRATCH_WIDTH
        sta     src+1
        tax
        tya
        jsr     pushax          ; push src to memcpy
        jmp     copy_row

copy_done:
        rts


update_progress_bar:
        lda     pgbar_state    ; Update progress bar
        clc
        adc     #4
        sta     pgbar_state
        bcc     :+
        inc     pgbar_state+1
        clc

:       ldy     #10
        jsr     subysp
        lda     #$FF

        dey                     ; -1,
        sta     (sp),y
        dey
        sta     (sp),y

        dey                     ; -1,
        sta     (sp),y
        dey
        sta     (sp),y

        dey                     ; 80*22,
        lda     #>(80*22)
        sta     (sp),y
        dey
        lda     #<(80*22)
        sta     (sp),y

        dey                     ; pgbar_state (long)
        lda     #0
        sta     (sp),y
        dey
        sta     (sp),y
        dey
        lda     pgbar_state+1
        sta     (sp),y
        dey
        lda     pgbar_state
        sta     (sp),y

        stz     sreg+1          ; height (long)
        stz     sreg
        lda     _height
        ldx     _height+1
        jmp     _progress_bar
