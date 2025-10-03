        .importzp        c_sp, sreg
        .importzp        tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4
        .importzp        _prev_rom_irq_vector, _prev_ram_irq_vector
        .importzp        _zp2, _zp3, _zp4p, _zp6p, _zp8p, _zp10p, _zp12, _zp13

        .import          _memcpy, _memset, _progress_bar
				.import          pushax, decsp4, subysp
        .import          _height
        .import          _width
        .import          _read, _ifd, _cache_end

        .import          floppy_motor_on

        .export          _raw_image
        .export          _magic
        .export          _model
        .export          _qt_load_raw
        .export          _cache
        .export          _cache_start

        .include         "fcntl.inc"

.macro UPDATE_BRANCH from, to
        .assert (to-from-2) >= -128, error
        .assert (to-from-2) <= 127, error
        lda     #<(to-from-2)
        sta     from+1

.endmacro

; Defines

BAND_HEIGHT   = 20
; QTKT algorithm relies on two pixels "borders" around the
; image to simplify bound checking.
SCRATCH_PAD   = 4
SCRATCH_WIDTH = (640 + SCRATCH_PAD + 1)
SCRATCH_HEIGHT= (BAND_HEIGHT + SCRATCH_PAD)
RAW_IMAGE_SIZE= (SCRATCH_HEIGHT * SCRATCH_WIDTH + 2)

; the column (X coord) loop is divided into an outer loop
; and an inner loop. the inner loop iterates over columns
; using the Y register so is limited to 255 pixels; as we
; need to iterate over 320 or 640 columns, the outer X
; loop shifts indexes by the inner loop len. We choose to
; iterate over 160 pixels in the inner loop as it is a
; multiple of 320 and 640 and makes bound checks simpler
; and faster.
; The algorithm will basically iterate in this way:
; for row = 20; row; row--
;  for outer_x = 4; outer_x; outer_x--
;   for Y = 0; Y != 160; Y++
;    ... handle pixel...
INNER_X_LOOP_LEN    = 160

.segment        "DATA"

_magic:
        .byte        $71,$6B,$74,$6B,$00
_model:
        .addr        model_str
_cache_start:
        .addr        _cache

.segment        "RODATA"
.align 256
; gstep correction (value byte for high nibble)
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

; gstep correction (value byte for low nibble)
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


; gstep correction (sign byte for low nibble)
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

; gstep correction sign byte for high nibble is derived from high nibble's high bit
; in the main loop so no array.

.align 256
START_INDEXES = *
; Index offsets for start of each row (low byte)
idx_low:
        .repeat BAND_HEIGHT+1,row
        .byte <(_raw_image + (22-row)*SCRATCH_WIDTH + (row .mod 2))
        ; We want to make sure no idx low byte == $00 so we can decrement it to compute idx_one
        .assert (_raw_image + (22-row)*SCRATCH_WIDTH + (row .mod 2)) .mod $100 <> $00,error
        .endrep

.assert >* = >START_INDEXES, error ; we switched a page
; Index offsets for start of each row (high byte)
idx_high:
        .repeat BAND_HEIGHT+1,row
        .byte >(_raw_image + (22-row)*SCRATCH_WIDTH + (row .mod 2))
        .endrep

.assert >* = >START_INDEXES, error ; we switched a page
; Forward index offsets for start of each row (low byte)
idx_forward_low:
        .repeat BAND_HEIGHT+1,row
        .byte <(_raw_image + (23-row)*SCRATCH_WIDTH - (row .mod 2) + 1)
        .endrep

.assert >* = >START_INDEXES, error ; we switched a page
; Forward index offsets for start of each row (high byte)
idx_forward_high:
        .repeat BAND_HEIGHT+1,row
        .byte >(_raw_image + (23-row)*SCRATCH_WIDTH - (row .mod 2) + 1)
        .endrep

.assert >* = >START_INDEXES, error ; we switched a page
; Backward index offsets for start of each row (low byte)
idx_behind_low:
        .repeat BAND_HEIGHT+1,row
        .byte <(_raw_image + (21-row)*SCRATCH_WIDTH + (row .mod 2) + 1)
        .endrep

.assert >* = >START_INDEXES, error ; we switched a page
; Backward index offsets for start of each row (high byte)
idx_behind_high:
        .repeat BAND_HEIGHT+1,row
        .byte >(_raw_image + (21-row)*SCRATCH_WIDTH + (row .mod 2) + 1)
        .endrep
END_INDEXES = *
.assert >END_INDEXES = >START_INDEXES, error ; we switched a page

; QTKT file magic
model_str:
        .byte        $31,$30,$30,$00

.segment        "BSS"
.align 1024
; Disk cache
_cache:
        .res        CACHE_SIZE,$00

.align 256
; Destination buffer for the band (8bpp grayscale)
_raw_image:
        .res        RAW_IMAGE_SIZE,$00

; No need to align anymore

; Status bar state, kept between bands
pgbar_state:
        .res        2,$00

; Whether to keep floppy motor on (patched to softswitch in the code)
motor_on:
        .res        2,$00

; Zero page pointers and variables

cur_cache_ptr     = _prev_ram_irq_vector ; Cache pointer, 2-bytes

row               = _zp2                 ; Current row, 1 byte
cur_row_loop      = _zp3                 ; Intra-row outer column loop counter

idx_one           = _zp4p                ; Pointers to destination buffer
idx               = _zp8p
idx_behind        = _zp10p
idx_behind2       = _prev_rom_irq_vector

ln_val            = _zp12                 ; Last low nibble computed value
hn_val            = _zp13                 ; Last high nibble computed value


; Offset to scratch start of last scratch lines, row 20 col 0
LAST_TWO_LINES = _raw_image + (BAND_HEIGHT * SCRATCH_WIDTH)

.segment        "CODE"

CACHE_END = _cache + CACHE_SIZE
.assert <CACHE_END = 0, error

; Patcher for direct load, and end-of-cache high byte check.
set_cache_data:
        lda     cur_cache_ptr
        sta     cache_read
        lda     cur_cache_ptr+1
        sta     cache_read+1
        rts

; Cache filler
fill_cache:
        ; Push read fd
        jsr     decsp4
        ldy     #$03

        lda     #$00                    ; ifd is never going to be > 255
        sta     (c_sp),y
        dey
        lda     _ifd
        sta     (c_sp),y
        dey

        ; Push buffer
        lda     _cache_start+1
        sta     cache_read+1
        sta     (c_sp),y
        dey

        lda     _cache_start
        sta     cache_read
        sta     (c_sp),y

        ; Push count (CACHE_SIZE)
        lda     #<CACHE_SIZE
        ldx     #>CACHE_SIZE
        jmp     _read

; Main loop, decodes a band
_qt_load_raw:
        cmp     #$00                    ; Check if we're on the first band
        bne     not_top
        cpx     #$00
        bne     not_top

top:
        sta     pgbar_state             ; Zero progress bar (A=0 here)
        jsr     set_cache_data          ; Initialize cache things

        lda     #(640/INNER_X_LOOP_LEN) ; How many outer loops per row ?
        ldx     _width
        cpx     #<(320)
        bne     :+
        lda     #(320/INNER_X_LOOP_LEN)
:       sta     set_row_loops+1         ; Patch outer col loop bound

        ; Init the second line + 2 bytes of buffer with grey
        lda     #<(_raw_image+SCRATCH_WIDTH)
        ldx     #>(_raw_image+SCRATCH_WIDTH)
        jsr     pushax
        lda     #$80
        ldx     #$00
        jsr     pushax
        lda     #<(SCRATCH_WIDTH + 1)
        ldx     #>(SCRATCH_WIDTH + 1)
        jsr     _memset

        ; Set indexes for first pixel special case
        lda     idx_low+BAND_HEIGHT
        sta     store_idx_min2_first_pixel+1
        lda     idx_high+BAND_HEIGHT
        sta     store_idx_min2_first_pixel+2

        lda     idx_forward_low+BAND_HEIGHT
        sta     store_idx_forward_first_pixel+1
        lda     idx_forward_high+BAND_HEIGHT
        sta     store_idx_forward_first_pixel+2

        ; Set indexes for first row special case
        lda     idx_behind_low+BAND_HEIGHT
        ldx     idx_behind_high+BAND_HEIGHT
        clc
        adc     #2
        sta     idx_behind2
        bcc     :+
        inx
        clc
:       stx     idx_behind2+1

        ldy     #BAND_HEIGHT            ; We iterate over 20 rows
        sty     row

        lda     floppy_motor_on         ; Patch motor_on if we use a floppy
        beq     row_loop
        sta     start_floppy_motor+1
        lda     #$C0                    ; Firmware access space
        sta     start_floppy_motor+2

        jmp     row_loop

not_top:                                ; Subsequent bands
                                        ; Shift the last band's last line, plus 2 pixels,
                                        ; to second line of the new band.
        lda     #<(_raw_image+SCRATCH_WIDTH)
        ldx     #>(_raw_image+SCRATCH_WIDTH)
        jsr     pushax
        lda     #<(LAST_TWO_LINES+SCRATCH_WIDTH)
        ldx     #>(LAST_TWO_LINES+SCRATCH_WIDTH)
        jsr     pushax
        lda     #<(SCRATCH_WIDTH + 1)
        ldx     #>(SCRATCH_WIDTH + 1)
        jsr     _memcpy

        ldy     #BAND_HEIGHT            ; We iterate over 20 rows
        sty     row

row_loop:                               ; Row loop

        lda     idx_low,y               ; Set indexes (Y = row there)
        sta     idx
        sta     store_idx_min2_first_col+1
        ldx     idx_high,y
        stx     idx+1
        stx     store_idx_min2_first_col+2

        sec
        sbc     #1
        sta     idx_one
        stx     idx_one+1

        lda     idx_forward_low,y
        sta     store_idx_forward_first_col+1
        lda     idx_forward_high,y
        sta     store_idx_forward_first_col+2

        lda     idx_behind_low,y
        sta     idx_behind
        ldx     idx_behind_high,y
        stx     idx_behind+1

        ldy     #$00
        lda     (idx),y                 ; Remember previous value before shifting
        sta     ln_val                  ; index

set_row_loops:
        lda     #$FF                    ; Patched (# of outer loops per row)
        sta     cur_row_loop
        jmp     col_outer_loop

; --------------------------------------; Inlined helpers, close enough to branch

; Increment cache pointer page
inc_cache_high:
        inc     cache_read+1

        ; Check for cache almost-end and restart floppy
        ; Consider we have time to handle 1kB while the
        ; drive restarts
        ldx     cache_read+1
        cpx     #(>CACHE_END)-4
        bmi     :+
start_floppy_motor:
        sta     motor_on                ; Patched if on floppy

:       ; Check for cache end and refill cache
        cpx     #>CACHE_END
        bne     handle_byte
        pha
        sty     tmp1                    ; Backup index
        jsr     fill_cache
        ldy     tmp1                    ; Restore index
        pla
        jmp     handle_byte

; ------
; Clamp value to 8bits
clamp_high_nibble_low:
        lda     #$00
        beq     high_nibble_special_neg    ; Back to main loop

; ------
; Handle first pixel special case
handle_first_pixel:
        sta     (idx_one),y
store_idx_forward_first_pixel:
        sta     $FFFF                   ; Patched
store_idx_min2_first_pixel:
        sta     $FFFF                   ; Patched
        sta     (idx_behind),y          ; *(idx_behind+2) = *(idx_behind+4) = val;
        sta     (idx_behind2),y

        sta     hn_val

        ; compute new branch offset (from high_nibble_special to first_row)
        UPDATE_BRANCH high_nibble_special_neg, handle_first_row_high
        UPDATE_BRANCH high_nibble_special_pos, handle_first_row_high
        lda     #$90                    ; BCC
        sta     high_nibble_special_pos

        lda     hn_val
        jmp     do_low_nibble           ; Back to main loop

; ------
; Handle first row's special case, for high nibble
handle_first_row_high:
        sta     (idx_behind),y          ; *(idx_behind+2) = *(idx_behind+4) = val;
        sta     (idx_behind2),y
        jmp     high_nibble_end         ; Back to main loop

; ------
; Handle first column special case
handle_first_col:
        sta     (idx_one),y
store_idx_forward_first_col:
        sta     $FFFF                   ; Patched
store_idx_min2_first_col:
        sta     $FFFF                   ; Patched

        sta     hn_val

        ; compute new branch offsets (from high_nibble_special to high_nibble_end)
        UPDATE_BRANCH high_nibble_special_neg, high_nibble_end
        ;UPDATE_BRANCH high_nibble_special_pos, high_nibble_end
        lda     #$B0                    ; BCS, disable jump
        sta     high_nibble_special_pos

        lda     hn_val
        jmp     do_low_nibble

; --------------------------------------; End of inlined helpers

col_outer_loop:                         ; Outer column loop, iterating 2 or 4 times
        ldy    #0

col_inner_loop:                         ; Inner column loop, iterating over Y
cache_read = *+1
        lda     $FFFF
        inc     cache_read
        beq     inc_cache_high          ; Increment cache ptr page and refill if needed

handle_byte:
        tax                             ; Get gstep vals to X (keep it in X!)

        ; HIGH NIBBLE
        ; val = ((((*idx_behind
        ;         + ln_val) >> 1)
        ;         + *(idx_behind+2)) >> 1)
        ;         + gstep[high_nibble];


        bmi     gstep_high_pos          ; gstep add simplification depending
                                        ; on the sign of its high byte

gstep_high_neg:
        lda     (idx_behind),y          ; (*idx_behind)
        adc     ln_val                  ; + ln_val
        ror                             ; >> 1
        clc

        iny
        iny
        adc     (idx_behind),y          ; + *(idx_behind+2)
        ror                             ; >> 1

        clc                             ; + gstep[h].
        adc     high_nibble_gstep_low,x ; Sets carry if overflow

        bcc     clamp_high_nibble_low   ; Clamp low as gstep is negative
        clc
high_nibble_special_neg:
        bcc     handle_first_pixel      ; (bra) Patched for special cases (first_pixel, then first_row, then first_col or high_nibble_end)

gstep_high_pos:
        lda     (idx_behind),y          ; (*idx_behind)
        adc     ln_val                  ; + ln_val
        ror                             ; >> 1
        clc

        iny
        iny
        adc     (idx_behind),y          ; + *(idx_behind+2)
        ror                             ; >> 1

        clc                             ; + gstep[h].
        adc     high_nibble_gstep_low,x
                                        ; Sets carry if overflow
        bcs     clamp_high_nibble_high

high_nibble_special_pos:
        bcc     handle_first_pixel      ; Patched for special cases (first_pixel, then first_row, then first_col or high_nibble_end)

high_nibble_end:
        sta     hn_val                  ; Done on not-first columns:
        adc     ln_val                  ; *(idx+1) = (val + ln_val) >> 1;
        ror                             ; (handle_first_col will branch back to do_low_nibble)
        clc
        sta     (idx_one),y
        lda     hn_val

        ; REPEAT WITH LOW NIBBLE (same, with +2 offsets on Y)

do_low_nibble:
        ; val = ((((*(idx_behind+2)
        ;         + hn_val) >> 1)
        ;         + *(idx_behind+4)) >> 1)
        ;         + gstep[low_nibble];
        sta     (idx),y                 ; store *(idx+2) = val, we didn't do it earlier
        adc     (idx_behind),y
        ror
        clc

        iny
        iny
        adc     (idx_behind),y
        ror
        clc

        adc     low_nibble_gstep_low,x  ; Sets carry if overflow

        sta     ln_val                  ; Store low nibble value

        lda     low_nibble_gstep_high,x ; Carry set by previous adc if overflowed
        adc     #0                      ; If A = $FF and C set, C will stay set
        clc                             ; so clear it.
        bne     clamp_low_nibble        ; Overflow - handle it (will rewrite and reload ln_val)
        lda     ln_val                  ; otherwise reload ln_val

low_nibble_special:
        bcc     handle_first_row_low    ; Patched (first_row, then low_nibble_end)

low_nibble_end:
        sta     (idx),y                 ; We have ln_val in A, store it at idx
        adc     hn_val                  ; and handle idx+1
        ror                             ; (don't bother to clc, carry cleared by cpy just after)
        sta     (idx_one),y
        ; END LOW NIBBLE

check_col_inner_loop:
        cpy     #(INNER_X_LOOP_LEN)     ; End of inner column loop?
        bne     col_inner_loop

shift_indexes:                          ; Yes so shift indexes for next inner loop
        lda     #(INNER_X_LOOP_LEN-1)   ; idx += INNER_X_LOOP_LEN
        adc     idx                     ; Adding INNER_X_LOOP_LEN-1 because
        sta     idx                     ; carry is set by the previous cpy
        bcc     :+
        inc     idx+1                   ; Don't clear carry, for idx_one

:       dec     cur_row_loop            ; Are we at end of line?
        beq     end_of_row              ; If so, no need to update idx_one and idx_behind

        tax
        dex
        stx     idx_one                 ; idx_one = idx-1
        bcc     :+
        inc     idx_one+1
        clc

:       tya                             ; Y = INNER_X_LOOP_LEN
        adc     idx_behind              ; idx_behind += INNER_X_LOOP_LEN
        sta     idx_behind
        bcc     shift_ib2
        inc     idx_behind+1
        clc

shift_ib2:                              ; Only done on first row, then patched out
        jmp     shift_extra_indexes     ; to jmp col_outer_loop

end_of_row:
        lda     ln_val                  ; *(idx+2) = val
        ldy     #2
        sta     (idx),y
        clc
first_row_special:                      ; Update special handlers at end of first row
        bcc     finish_first_row        ; (will be patched out)


; --------------------------------------; Inlined helpers, close enough to branch
; Handle end of row
row_done:
        ; Put back first col handler for high nibble
        UPDATE_BRANCH high_nibble_special_neg, handle_first_col
        UPDATE_BRANCH high_nibble_special_pos, handle_first_col
        lda     #$90                    ; BCC
        sta     high_nibble_special_pos

        ldy     row
        dey
        beq     band_done
        sty     row
        jmp     row_loop

; ------
; Clamp value to 8 bits
clamp_high_nibble_high:
        lda     #$FF
        clc
        bcc     high_nibble_special_pos    ; Back to main loop

clamp_low_nibble:
        eor     #$FF                     ; => 00 if negative, FE if positive
        bpl     :+
        lda     #$FF                     ; => FF if positive

:       sta     ln_val
low_nibble_clamped:
        bcc     handle_first_row_low    ; Carry sure to be clear from where we come

        ; First cols and first row handlers, out of main loop

; ------
; Handle first row special case (low nibble)
handle_first_row_low:
        sta     (idx_behind),y           ; *(idx_behind+4) = *(idx_behind+6) = val;
        sta     (idx_behind2),y
        jmp     low_nibble_end

        ; End of first col/row handlers

; ------
; Handle end of first row
finish_first_row:
        lda     #$B0                    ; BCS - always false at low_nibble_special
        sta     low_nibble_special
        sta     first_row_special       ; Unplug first row handler
        UPDATE_BRANCH low_nibble_clamped, low_nibble_end

        lda     #<col_outer_loop        ; unplug idx_behind2 updater
        sta     shift_ib2+1             ; jmp col_outer_loop
        lda     #>col_outer_loop
        sta     shift_ib2+2
        jmp     row_done

shift_extra_indexes:
        tya                             ; Y = INNER_X_LOOP_LEN
        adc     idx_behind2             ; idx_behind += INNER_X_LOOP_LEN
        sta     idx_behind2
        bcc     :+
        inc     idx_behind2+1
        clc
:       jmp     col_outer_loop

; ------
; Update progress bar at end of band and return
band_done:
        lda     pgbar_state             ; Update progress bar
        clc
        adc     #BAND_HEIGHT
        sta     pgbar_state
        bcc     :+
        inc     pgbar_state+1
        clc

:       ldy     #10
        jsr     subysp
        lda     #$FF

        dey                              ; -1,
        sta     (c_sp),y
        dey
        sta     (c_sp),y

        dey                              ; -1,
        sta     (c_sp),y
        dey
        sta     (c_sp),y

        dey                              ; 80*22,
        lda     #>(80*22)
        sta     (c_sp),y
        dey
        lda     #<(80*22)
        sta     (c_sp),y

        dey                              ; pgbar_state (long)
        lda     #0
        sta     (c_sp),y
        dey
        sta     (c_sp),y
        dey
        lda     pgbar_state+1
        sta     (c_sp),y
        dey
        lda     pgbar_state
        sta     (c_sp),y

        lda     #$00
        sta     sreg+1                   ; height (long)
        sta     sreg
        lda     _height
        ldx     _height+1
        jmp     _progress_bar
