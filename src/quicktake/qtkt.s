        .importzp        sp, sreg
        .importzp        tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4
        .importzp        _prev_rom_irq_vector, _prev_ram_irq_vector, _zp6p, _zp8p, _zp10p, _zp12, _zp13, _zp12ip

        .import          _memcpy, _memset, _progress_bar
				.import          pushax, pusha, pusha0, decsp6, incsp6, subysp
        .import          _height
        .import          _width
        .import          _fread, _ifp, _cache_end

        .export          _raw_image
        .export          _magic
        .export          _model
        .export          _huff_ptr
        .export          _qt_load_raw
        .export          _cache
        .export          _cache_start

; Defines

BAND_HEIGHT   = 20
SCRATCH_PAD   = 4
SCRATCH_WIDTH = (640 + SCRATCH_PAD)
SCRATCH_HEIGHT= (BAND_HEIGHT + SCRATCH_PAD)
PIXELBUF_SIZE = (SCRATCH_HEIGHT * SCRATCH_WIDTH + 2)
RAW_IMAGE_SIZE= BAND_HEIGHT * 640

Y_LOOP_LEN    = 160

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

clamper:.byte $00,$80,$FF

high_nibble_gstep_low:
        .repeat 16
        .byte        $A7
        .endrep
        .repeat 16
        .byte        $C4
        .endrep
        .repeat 16
        .byte        $D4
        .endrep
        .repeat 16
        .byte        $E0
        .endrep
        .repeat 16
        .byte        $EA
        .endrep
        .repeat 16
        .byte        $F1
        .endrep
        .repeat 16
        .byte        $F8
        .endrep
        .repeat 16
        .byte        $FE
        .endrep
        .repeat 16
        .byte        $02
        .endrep
        .repeat 16
        .byte        $08
        .endrep
        .repeat 16
        .byte        $0F
        .endrep
        .repeat 16
        .byte        $16
        .endrep
        .repeat 16
        .byte        $20
        .endrep
        .repeat 16
        .byte        $2C
        .endrep
        .repeat 16
        .byte        $3C
        .endrep
        .repeat 16
        .byte        $59
        .endrep

high_nibble_gstep_high:
        .repeat 128
        .byte        $FF
        .endrep
        .repeat 128
        .byte        $00
        .endrep

low_nibble_gstep_low:
        .repeat 16
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
        .endrep

low_nibble_gstep_high:
        .repeat 16
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
        .endrep

model_str:
        .byte        $31,$30,$30,$00

.segment        "BSS"
.align 256
_raw_image:
        .res        RAW_IMAGE_SIZE,$00
.align 256
pixelbuf:
        .res        PIXELBUF_SIZE,$00

_cache:
        .res        CACHE_SIZE,$00
dst:
        .res        2,$00
pgbar_state:
        .res        2,$00

; Offset to scratch start of last scratch lines, row 20 col 0
LAST_TWO_LINES = pixelbuf + (BAND_HEIGHT * SCRATCH_WIDTH)

; Offset to real start of third line, row 2 col 2
THIRD_LINE     = pixelbuf + (2 * SCRATCH_WIDTH) + 2


.segment        "CODE"

cur_cache_ptr = _prev_ram_irq_vector

_reset_bitbuff:
        ; Patch end-of-cache comparisons
        lda     _cache_end
        sta     cache_check_low_byte+1
        lda     _cache_end+1
        sta     cache_check_high_byte+1
        rts

fill_cache:
        jsr     decsp6

        ; Push fread dest pointer
        ldy     #$05

        lda     _cache_start+1
        sta     cur_cache_ptr+1
        sta     (sp),y

        lda     _cache_start
        sta     cur_cache_ptr
        dey
        sta     (sp),y

        ; Push size (1)
        dey
        lda     #0
        sta     (sp),y
        dey
        inc     a
        sta     (sp),y

        ; Push count ($1000, CACHE_SIZE)
        dey
        lda     #>CACHE_SIZE
        sta     (sp),y
        dey
        lda     #<CACHE_SIZE
        sta     (sp),y

        lda     _ifp
        ldx     _ifp+1
        jmp     _fread


val               = _prev_rom_irq_vector
row               = _prev_rom_irq_vector+1

src               = _zp6p
idx               = _zp8p
idx_behind        = _zp10p
last_val          = _zp12

; Don't put this one in LC as it is patched on runtime
_qt_load_raw:
        cmp     #$00
        bne     :+
        cpx     #$00
        beq     top
:       jmp     not_top

top:    jsr     _reset_bitbuff  ; Yes. Initialize things

        stz     pgbar_state

        ; Fill whole buffer with grey
        lda     #<(pixelbuf)
        ldx     #>(pixelbuf)
        jsr     pushax
        lda     #$80
        jsr     pusha0
        lda     #<PIXELBUF_SIZE
        ldx     #>PIXELBUF_SIZE
        jsr     _memset

        lda     #1
        sta     check_first_row+1  ; Init with non-zero value
        sta     check_first_row2+1  ; Init with non-zero value

        jmp     start_work
not_top:
        ; Shift the previous band's last two lines, plus 2 pixels,
        ; to the start of the new band.
        lda     #<(pixelbuf)
        ldx     #>(pixelbuf)
        jsr     pushax
        lda     #<(LAST_TWO_LINES)
        ldx     #>(LAST_TWO_LINES)
        jsr     pushax
        lda     #<(2*SCRATCH_WIDTH + 2)
        ldx     #>(2*SCRATCH_WIDTH + 2)
        jsr     _memcpy

        ; Reset the rest of the lines with grey
        lda     #<(THIRD_LINE)
        ldx     #>(THIRD_LINE)
        jsr     pushax
        lda     #$80
        jsr     pusha0
        lda     #<(PIXELBUF_SIZE-(2*SCRATCH_WIDTH + 2))
        ldx     #>(PIXELBUF_SIZE-(2*SCRATCH_WIDTH + 2))
        jsr     _memset

start_work:
        ; We start at line 2
        lda     #>(pixelbuf + (2 * SCRATCH_WIDTH))
        sta     src+1
        lda     #<(pixelbuf + (2 * SCRATCH_WIDTH))
        sta     src

        ; We iterate over 20 lines
        lda     #BAND_HEIGHT
        sta     row

first_pass_next_row:
        lda     row             ; Row & 1?
        bit     #$01
        beq     even_row

        clc
        lda     src             ; idx_end = src + width + 1
        adc     _width
        tay
        lda     src+1
        adc     _width+1
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
        lda     src             ; idx_end = src + width
        adc     _width
        sta     check_first_pass_col_loop+1
        lda     src+1
        adc     _width+1
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

        sec                     ; Set idx_behind = idx - (SCRATCH_WIDTH-1)
        sbc     #<(SCRATCH_WIDTH-1)
        sta     idx_behind
        txa
        sbc     #>(SCRATCH_WIDTH-1)
        sta     idx_behind+1

        clc
        lda     #<SCRATCH_WIDTH ; src += SCRATCH_WIDTH
        adc     src
        sta     src
        lda     #>SCRATCH_WIDTH
        adc     src+1
        sta     src+1

        ; We're at first column
        sta     check_first_col+1

        jmp     first_pass_col_loop

cache_check_high_byte:
        ldx     #0              ; Patched when resetting (_cache_end+1)
        cpx     cur_cache_ptr+1
        bne     fetch_byte
        phy
        jsr     fill_cache
        ply
        jmp     fetch_byte

clamp_high_nibble:
        eor     #$FF            ; => 00 if negative, FE if positive
        bpl     :+
        lda     #$FF            ; => FF if positive

:       sta     (idx),y         ; *(idx+2) = val
        sta     last_val
        jmp     check_first_col

first_pass_col_loop:

        ldy    #0

first_pass_col_y_loop:
cache_check_low_byte:
        lda     #0              ; Patched when resetting (_cache_end)
        cmp     cur_cache_ptr
        beq     cache_check_high_byte

fetch_byte:
        lda     (cur_cache_ptr)

        inc     cur_cache_ptr
        bne     :+
        inc     cur_cache_ptr+1

:       tax                     ; Get gstep vals to X (keep it in X!)

        ; HIGH NIBBLE
        ; val = ((((*idx_behind
        ;         + last_val) >> 1)
        ;         + *(idx_behind+2)) >> 1)
        ;         + gstep[high_nibble];

        clc
        lda     (idx_behind),y  ; (*idx_behind)
        adc     last_val        ; + last_val
        ror                     ; >> 1

        clc

        iny
        iny
        adc     (idx_behind),y  ; + *(idx_behind+2)
        ror                     ; >> 1

        clc                     ; + gstep[h].
        adc     high_nibble_gstep_low,x
                                ; Sets carry if overflow

        sta     (idx),y         ; *(idx+2) = val
        sta     last_val        ; last_val = val

        lda     high_nibble_gstep_high,x
                                ; Carry set by previous adc if overflowed

        adc     #0
        bne     clamp_high_nibble; overflow
        clc

check_first_col:
        lda     #1              ; Patched
        bne     handle_first_col

check_first_row:
        lda     #1              ; Patched
        bne     handle_first_row_high_nibble

        ; REPEAT WITH LOW NIBBLE (same, with +2 offsets on Y)

do_low_nibble:
        ; val = ((((*(idx_behind+2)
        ;         + last_val) >> 1)
        ;         + *(idx_behind+4)) >> 1)
        ;         + gstep[low_nibble];

        lda     (idx_behind),y  ; Y expected to be 2 there
        adc     last_val
        ror

        clc

        iny
        iny
        adc     (idx_behind),y
        ror

        clc                     ; + gstep[h].
        adc     low_nibble_gstep_low,x
                                ; Sets carry if overflow

        sta     (idx),y         ; *(idx+4) = val
        sta     last_val

        lda     low_nibble_gstep_high,x
                                ; Carry set by previous adc if overflowed

        adc     #0
        bne     clamp_low_nibble; Overflow

check_first_row2:
        lda     #1              ; Patched
        bne     handle_first_row_low_nibble

        ; END LOW NIBBLE

check_first_pass_col_y_loop:
        cpy     #Y_LOOP_LEN
        bne     first_pass_col_y_loop

shift_indexes:
        clc                     ; idx_behind += 4
        lda     idx_behind
        adc     #Y_LOOP_LEN
        sta     idx_behind
        bcc     :+
        inc     idx_behind+1
        clc

:       lda     idx             ; idx += 4
        adc     #Y_LOOP_LEN
        sta     idx
        bcc     check_first_pass_col_loop
        inc     idx+1

check_first_pass_col_loop:
        cmp     #0              ; Patched (idx_end)
        beq     :+
        jmp     first_pass_col_loop
:       lda     idx+1
check_first_pass_col_loop_hi:
        cmp     #0              ; Patched (idx_end+1)
        beq     end_of_line
        jmp     first_pass_col_loop

end_of_line:
        lda     last_val        ; *(idx+2) = val
        ldy     #2
        sta     (idx),y
        stz     check_first_row+1
        stz     check_first_row2+1

        dec     row
        beq     start_second_pass
        jmp     first_pass_next_row

        ; First cols and first row handlers, out of main loop

clamp_low_nibble:
        beq     check_first_row2; No overflow
        eor     #$FF            ; => 00 if negative, FE if positive
        bpl     :+
        lda     #$FF            ; => FF if positive

:       sta     (idx),y         ; *(idx+4) = val
        sta     last_val
        jmp     check_first_pass_col_y_loop

handle_first_col:
        lda     last_val
store_idx_forward:
        sta     $FFFF           ; Patched
store_idx_min2:
        sta     $FFFF           ; Patched
        stz     check_first_col+1
        jmp     check_first_row

handle_first_row_high_nibble:
        lda     last_val        ; get val back
        sta     (idx_behind),y  ; *(idx_behind+2) = *(idx_behind+4) = val;
        iny
        iny
        sta     (idx_behind),y
        dey
        dey                     ; Set Y back for low nibble
        jmp     do_low_nibble

handle_first_row_low_nibble:
        lda     last_val        ; get val back
        sta     (idx_behind),y  ; *(idx_behind+4) = *(idx_behind+6) = val;
        iny
        iny
        sta     (idx_behind),y
        dey
        dey
        jmp     check_first_pass_col_y_loop

        ; End of first col/row handlers


start_second_pass:
        lda     #>(pixelbuf + (2 * SCRATCH_WIDTH))
        sta     src+1
        lda     #<(pixelbuf + (2 * SCRATCH_WIDTH))
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
        tya
        sta     idx
        stx     idx+1

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

        ldy     #0

second_pass_col_y_loop:
        ldx     #0              ; Overflow counter
        clc
        lda     (idx),y         ; *(idx)
        iny
        iny
        adc     (idx),y         ; *(idx+2)
        ror                     ; >> 1 and get carry back to high bit
        sta     tmp1

        dey
        lda     (idx),y         ; *(idx+1) << 1
        asl

        bcc     :+
        inx
        clc

:       adc     tmp1
        bcc     :+
        inx

        ; Now X = 0 means val < 0, X = 1 means val in range, X = 2 means val > 255
:       cpx     #1
        beq     :+
        lda     clamper,x

:       sta     (idx),y         ; *(idx+1) = val

        iny
        cpy     #Y_LOOP_LEN
        bne     second_pass_col_y_loop
        
        ; Shift index
        lda     idx
        clc
        adc     #Y_LOOP_LEN
        sta     idx
        bcc     check_second_pass_col_loop
        inc     idx+1

check_second_pass_col_loop:
        ; Are we done for this row?
        cmp     #0              ; Patched (idx_end)
        beq     :+
        jmp     second_pass_col_loop
:       ldx     idx+1
check_second_pass_col_loop_hi:
        cpx     #0              ; Patched (idx_end+1)
        beq     second_pass_row_done
        jmp     second_pass_col_loop

second_pass_row_done:
        dec     row
        beq     :+
        jmp     second_pass_next_row

        ; Both passes done, memcpy BAND_HEIGHT lines to destination buffer,
        ; excluding two leftmost and rightmost scratch pixels 
:       lda     #<(_raw_image)
        sta     idx
        ldx     #>(_raw_image)
        stx     idx+1

        clc
        lda     #<(pixelbuf + (2 * SCRATCH_WIDTH))
        ldx     #>(pixelbuf + (2 * SCRATCH_WIDTH))
        adc     #2
        sta     src
        bcc     :+
        inx
        clc
:       stx     src+1

        lda     #BAND_HEIGHT
        sta     row

        lda     _width
        cmp     #<(640)
        bne     next_row_320

next_row_640:
        ldx     #2              ; Two pages
page_640:
        ldy     #0
:
.repeat 4                       ; One page
        lda     (src),y
        sta     (idx),y
        dey
.endrep
        bne     :-
        inc     src+1
        inc     idx+1
        dex
        bne     page_640

        ldy     #<(640-(256*2)-1) ; Last part
:       lda     (src),y
        sta     (idx),y
        dey
        bpl     :-

        clc
        lda     idx
        adc     #<(640-(256*2))
        sta     idx
        bcc     :+
        inc     idx+1
        clc

:       lda     src
        adc     #<(640-(256*2)+SCRATCH_PAD)
        sta     src
        bcc     :+
        inc     src+1
        clc
:       dec     row
        bne     next_row_640
        rts

next_row_320:
        ldy     #00
:
.repeat 4                       ; One page
        lda     (src),y
        sta     (idx),y
        dey
.endrep
        bne     :-
        inc     src+1
        inc     idx+1

        ldy     #<(320-(256*2)-1) ; Last part
:       lda     (src),y
        sta     (idx),y
        dey
        bpl     :-

        clc
        lda     idx
        adc     #<(320-(256*2))
        sta     idx
        bcc     :+
        inc     idx+1
        clc

:       lda     src
        adc     #<(640-256+SCRATCH_PAD)
        sta     src
        lda     src+1           ; Finish add, can cross two pages
        adc     #>(640-256+SCRATCH_PAD)
        sta     src+1
        dec     row
        bne     next_row_320
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
