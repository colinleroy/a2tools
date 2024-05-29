        .importzp        sp, sreg
        .importzp        tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4
        .importzp        _prev_rom_irq_vector, _prev_ram_irq_vector
        .importzp        _zp4p, _zp6p, _zp8p, _zp10p, _zp12, _zp13

        .import          _memcpy, _memset, _progress_bar
				.import          pushax, pusha, pusha0, decsp6, incsp6, subysp
        .import          _height
        .import          _width
        .import          _fread, _ifp, _cache_end

        .import          floppy_motor_on

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
RAW_IMAGE_SIZE= (SCRATCH_HEIGHT * SCRATCH_WIDTH + 2)

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
.align 1024
_cache:
        .res        CACHE_SIZE,$00
.align 256
_raw_image:
        .res        RAW_IMAGE_SIZE,$00
.align 256
dst:
        .res        2,$00
pgbar_state:
        .res        2,$00
motor_on:
        .res        2,$00

; Offset to scratch start of last scratch lines, row 20 col 0
LAST_TWO_LINES = _raw_image + (BAND_HEIGHT * SCRATCH_WIDTH)

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

        ; Push count (CACHE_SIZE)
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

idx_one           = _zp4p
src               = _zp6p
idx               = _zp8p
idx_behind        = _zp10p
ln_val            = _zp12
hn_val            = _zp13

; Don't put this one in LC as it is patched on runtime
_qt_load_raw:
        cmp     #$00
        bne     not_top
        cpx     #$00
        bne     not_top

top:    jsr     _reset_bitbuff  ; Yes. Initialize things

        stz     pgbar_state

        ; Init the second line + 2 bytes of buffer with grey
        lda     #<(_raw_image+SCRATCH_WIDTH)
        ldx     #>(_raw_image+SCRATCH_WIDTH)
        jsr     pushax
        lda     #$80
        ldx     #$00
        jsr     pushax
        lda     #<(SCRATCH_WIDTH + 2)
        ldx     #>(SCRATCH_WIDTH + 2)
        jsr     _memset

        lda     #$4C              ; handle first row - JMP
        sta     check_first_row
        sta     check_first_row2

        lda     floppy_motor_on    ; Patch motor-on if we use a floppy
        beq     start_work
        sta     keep_motor_on+1
        sta     keep_motor_on_beg+1
        lda     #$C0
        sta     keep_motor_on+2
        sta     keep_motor_on_beg+2

keep_motor_on_beg:
        sta     motor_on        ; Keep drive motor running

        jmp     start_work
not_top:
        ; Shift the last band's last line, plus 2 pixels,
        ; to second line of the new band.
        lda     #<(_raw_image+SCRATCH_WIDTH)
        ldx     #>(_raw_image+SCRATCH_WIDTH)
        jsr     pushax
        lda     #<(LAST_TWO_LINES+SCRATCH_WIDTH)
        ldx     #>(LAST_TWO_LINES+SCRATCH_WIDTH)
        jsr     pushax
        lda     #<(SCRATCH_WIDTH + 2)
        ldx     #>(SCRATCH_WIDTH + 2)
        jsr     _memcpy

start_work:
        ; We start at line 2
        lda     #>(_raw_image + (2 * SCRATCH_WIDTH))
        sta     src+1
        lda     #<(_raw_image + (2 * SCRATCH_WIDTH))
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
        tay
        adc     #<(SCRATCH_WIDTH+1)
        sta     store_idx_forward+1
        lda     src+1
        sta     idx+1
        tax
        adc     #>(SCRATCH_WIDTH+1)
        sta     store_idx_forward+2

first_pass_row_work:
        cpy     #$00
        bne     :+
        dex
:       dey
        sty     idx_one
        stx     idx_one+1

        lda     (idx)           ; Remember previous val before shifting
        sta     ln_val          ; index

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
        lda     #$4C               ; JMP
        sta     check_first_col

        jmp     first_pass_col_loop

cache_check_high_byte:
        ldx     #0              ; Patched when resetting (_cache_end+1)
        cpx     cur_cache_ptr+1
        bne     fetch_byte
        phy
        jsr     fill_cache
        ply
keep_motor_on:
        sta     motor_on        ; Keep drive motor running
        jmp     fetch_byte

clamp_high_nibble:
        eor     #$FF            ; => 00 if negative, FE if positive
        bpl     :+
        lda     #$FF            ; => FF if positive

:       sta     hn_val
        jmp     store_high_nibble

handle_first_row_high_nibble:
        sta     (idx_behind),y  ; *(idx_behind+2) = *(idx_behind+4) = val;
        iny
        iny
        sta     (idx_behind),y
        dey
        dey                     ; Set Y back for low nibble
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
        ;         + ln_val) >> 1)
        ;         + *(idx_behind+2)) >> 1)
        ;         + gstep[high_nibble];

        clc
        lda     (idx_behind),y  ; (*idx_behind)
        adc     ln_val          ; + ln_val
        ror                     ; >> 1

        clc

        iny
        iny
        adc     (idx_behind),y  ; + *(idx_behind+2)
        ror                     ; >> 1

        clc                     ; + gstep[h].
        adc     high_nibble_gstep_low,x
                                ; Sets carry if overflow

                                ; Write them now even if overwritten in clamp_*nibble - it's faster
        sta     hn_val          ; hn_val = val

        lda     high_nibble_gstep_high,x
                                ; Carry set by previous adc if overflowed

        adc     #0              ; If A = $FF and C set, C will stay set
        clc                     ; so clear it.
        bne     clamp_high_nibble; overflow - handle it
        lda     hn_val          ; Otherwise reload hn_val

store_high_nibble:
        sta     (idx),y         ; First store high nibble's result

check_first_row:
        jmp     handle_first_row_high_nibble ; Patched

check_first_col:
        jmp     handle_first_col; Patched

first_col_checked:
                                ; Not first col:
        adc     ln_val          ; *(idx+1) = (val + ln_val) >> 1;
        ror
        clc
        sta     (idx_one),y

        ; REPEAT WITH LOW NIBBLE (same, with +2 offsets on Y)

do_low_nibble:
        ; val = ((((*(idx_behind+2)
        ;         + hn_val) >> 1)
        ;         + *(idx_behind+4)) >> 1)
        ;         + gstep[low_nibble];

        lda     hn_val          ; Reload hn_val
        adc     (idx_behind),y  ; Y expected to be 2 there
        ror
        clc

        iny
        iny
        adc     (idx_behind),y
        ror

        clc                     ; + gstep[h].
        adc     low_nibble_gstep_low,x
                                ; Sets carry if overflow

        sta     ln_val

        lda     low_nibble_gstep_high,x
                                ; Carry set by previous adc if overflowed

        adc     #0              ; If A = $FF and C set, C will stay set
        clc                     ; so clear it.
        bne     clamp_low_nibble; Overflow - handle it
        lda     ln_val          ; otherwise reload ln_val

check_first_row2:
        jmp     handle_first_row_low_nibble ; Patched

low_nibble_end:                 ; We have ln_val in A, store it
        sta     (idx),y         ; at idx
        adc     hn_val          ; and handle idx+1
        ror
        clc
        sta     (idx_one),y
        ; END LOW NIBBLE

check_first_pass_col_y_loop:
        cpy     #Y_LOOP_LEN
        bne     first_pass_col_y_loop

shift_indexes:
        clc                     ; idx_behind += Y_LOOP_LEN
        tya                     ; Y = Y_LOOP_LEN here
        adc     idx_behind
        sta     idx_behind
        bcc     :+
        inc     idx_behind+1
        clc

:       tya
        adc     idx_one             ; idx_one += Y_LOOP_LEN
        sta     idx_one
        bcc     :+
        inc     idx_one+1
        clc

:       tya
        adc     idx             ; idx += Y_LOOP_LEN
        sta     idx             ; increment idx last for bound checking
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
        lda     ln_val          ; *(idx+2) = val
        ldy     #2
        sta     (idx),y
        lda     #$2C
        sta     check_first_row
        sta     check_first_row2

        dec     row
        beq     :+
        jmp     first_pass_next_row
:       rts

        ; First cols and first row handlers, out of main loop

handle_first_col:
        sta     (idx_one),y
store_idx_forward:
        sta     $FFFF           ; Patched
store_idx_min2:
        sta     $FFFF           ; Patched
        lda     #$2C
        sta     check_first_col ; Unplug first col handler
        jmp     do_low_nibble

clamp_low_nibble:
        eor     #$FF            ; => 00 if negative, FE if positive
        bpl     :+
        lda     #$FF            ; => FF if positive

:       sta     ln_val
        jmp     check_first_row2

handle_first_row_low_nibble:
        sta     (idx_behind),y  ; *(idx_behind+4) = *(idx_behind+6) = val;
        iny
        iny
        sta     (idx_behind),y
        dey
        dey
        jmp     low_nibble_end

        ; End of first col/row handlers

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
