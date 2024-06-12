        .importzp        sp, sreg
        .importzp        tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4
        .importzp        _prev_rom_irq_vector, _prev_ram_irq_vector
        .importzp        _zp2, _zp3, _zp4p, _zp6p, _zp8p, _zp10p, _zp12, _zp13

        .import          _memcpy, _memset, _progress_bar
				.import          pushax, decsp6, incsp6, subysp
        .import          _height
        .import          _width
        .import          _fread, _ifp, _cache_end

        .import          floppy_motor_on

        .export          _raw_image
        .export          _magic
        .export          _model
        .export          _qt_load_raw
        .export          _cache
        .export          _cache_start

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
; Index offsets for start of each row (low byte)
idx_low:
        .repeat BAND_HEIGHT+1,row
        .byte <(_raw_image + (22-row)*SCRATCH_WIDTH + (row .mod 2))
        ; We want to make sure no idx low byte == $00 so we can decrement it to compute idx_one
        .assert (_raw_image + (22-row)*SCRATCH_WIDTH + (row .mod 2)) .mod $100 <> $00,error
        .endrep

; Index offsets for start of each row (high byte)
idx_high:
        .repeat BAND_HEIGHT+1,row
        .byte >(_raw_image + (22-row)*SCRATCH_WIDTH + (row .mod 2))
        .endrep

.align 256
; Forward index offsets for start of each row (low byte)
idx_forward_low:
        .repeat BAND_HEIGHT+1,row
        .byte <(_raw_image + (23-row)*SCRATCH_WIDTH - (row .mod 2) + 1)
        .endrep

; Forward index offsets for start of each row (high byte)
idx_forward_high:
        .repeat BAND_HEIGHT+1,row
        .byte >(_raw_image + (23-row)*SCRATCH_WIDTH - (row .mod 2) + 1)
        .endrep

.align 256
; Backward index offsets for start of each row (low byte)
idx_behind_low:
        .repeat BAND_HEIGHT+1,row
        .byte <(_raw_image + (21-row)*SCRATCH_WIDTH + (row .mod 2) + 1)
        .endrep

; Backward index offsets for start of each row (high byte)
idx_behind_high:
        .repeat BAND_HEIGHT+1,row
        .byte >(_raw_image + (21-row)*SCRATCH_WIDTH + (row .mod 2) + 1)
        .endrep

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
.align 256

; Status bar state, kept between bands
pgbar_state:
        .res        2,$00

; Whether to keep floppy motor on (patched to softswitch in the code)
motor_on:
        .res        2,$00

; Zero page pointers and variables

row               = _prev_rom_irq_vector ; Current row, 1 byte
cur_cache_ptr     = _prev_ram_irq_vector ; Cache pointer, 2-bytes

cur_row_loop      = _zp3                 ; Intra-row outer column loop counter

idx_one           = _zp4p                ; Pointers to destination buffer
idx               = _zp8p
idx_behind        = _zp10p

ln_val            = _zp12                 ; Last low nibble computed value
hn_val            = _zp13                 ; Last high nibble computed value


; Offset to scratch start of last scratch lines, row 20 col 0
LAST_TWO_LINES = _raw_image + (BAND_HEIGHT * SCRATCH_WIDTH)

.segment        "CODE"

; Patcher for end-of-cache high byte check.
set_cache_end:
        lda     _cache_end
        beq     :+
        brk                             ; Make sure cache end is aligned
:       lda     _cache_end+1            ; Patch end-of-cache comparison
        sta     cache_check_high_byte+1
        rts

; Cache filler
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

; Main loop, decodes a band
_qt_load_raw:
        cmp     #$00                    ; Check if we're on the first band
        bne     not_top
        cpx     #$00
        bne     not_top

top:    jsr     set_cache_end           ; Yes. Initialize things

        stz     pgbar_state             ; Zero progress bar

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
        lda     #<(SCRATCH_WIDTH + 2)
        ldx     #>(SCRATCH_WIDTH + 2)
        jsr     _memset

        lda     idx_low+BAND_HEIGHT     ; Set indexes for first pixel special case
        sta     store_idx_min2_first_pixel+1
        lda     idx_high+BAND_HEIGHT
        sta     store_idx_min2_first_pixel+2

        lda     idx_forward_low+BAND_HEIGHT
        sta     store_idx_forward_first_pixel+1
        lda     idx_forward_high+BAND_HEIGHT
        sta     store_idx_forward_first_pixel+2

        ldy     #BAND_HEIGHT            ; We iterate over 20 rows
        sty     row

        lda     floppy_motor_on         ; Patch motor_on if we use a floppy
        beq     row_loop
        sta     keep_motor_on+1
        sta     keep_motor_on_beg+1
        lda     #$C0
        sta     keep_motor_on+2
        sta     keep_motor_on_beg+2

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
        lda     #<(SCRATCH_WIDTH + 2)
        ldx     #>(SCRATCH_WIDTH + 2)
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

        dec     a
        sta     idx_one
        stx     idx_one+1

        lda     idx_forward_low,y
        sta     store_idx_forward_first_col+1
        lda     idx_forward_high,y
        sta     store_idx_forward_first_col+2

        lda     idx_behind_low,y
        sta     idx_behind
        lda     idx_behind_high,y
        sta     idx_behind+1

        lda     (idx)                   ; Remember previous value before shifting
        sta     ln_val                  ; index

set_row_loops:
        lda     #0                      ; Patched (# of outer loops per row)
        sta     cur_row_loop
        bra     col_outer_loop

; --------------------------------------; Inlined helpers, close enough to branch

; Increment cache pointer page
inc_cache_high:
        inc     cur_cache_ptr+1
keep_motor_on_beg:
        sta     motor_on                ; Keep drive motor running
; Check for cache end and refill cache
cache_check_high_byte:
        ldx     #0                      ; Patched when resetting (_cache_end+1)
        cpx     cur_cache_ptr+1
        clc
        bne     handle_byte
        phy                             ; Backup needed registers before loading from disk
        pha
        jsr     fill_cache
        pla
        ply
keep_motor_on:
        sta     motor_on                ; Keep drive motor running
        bra     handle_byte

; ------
; Clamp value to 8bits
clamp_high_nibble_high:
        lda     #$FF
        clc
        bra     high_nibble_special_pos    ; Back to main loop

clamp_high_nibble_low:
        lda     #$00
        bra     high_nibble_special_pos    ; Back to main loop

; ------
; Handle first pixel special case
handle_first_pixel:
        sta     (idx_one),y
store_idx_forward_first_pixel:
        sta     $FFFF                   ; Patched
store_idx_min2_first_pixel:
        sta     $FFFF                   ; Patched
        sta     (idx_behind),y          ; *(idx_behind+2) = *(idx_behind+4) = val;
        iny
        iny
        sta     (idx_behind),y
        dey
        dey                             ; Set Y back for low nibble

        sta     hn_val
        ; compute new branch offset (from high_nibble_special to first_row)
        lda     #<(handle_first_row_high-high_nibble_special_neg-2)
        sta     high_nibble_special_neg+1
        lda     #<(handle_first_row_high-high_nibble_special_pos-2)
        sta     high_nibble_special_pos+1

        lda     hn_val
        bra     do_low_nibble           ; Back to main loop

; ------
; Handle first row's special case, for high nibble
handle_first_row_high:
        sta     (idx_behind),y          ; *(idx_behind+2) = *(idx_behind+4) = val;
        iny
        iny
        sta     (idx_behind),y
        dey
        dey                             ; Set Y back for low nibble
        bra     high_nibble_end         ; Back to main loop

; ------
; Handle first column special case
handle_first_col:
        sta     (idx_one),y
store_idx_forward_first_col:
        sta     $FFFF                   ; Patched
store_idx_min2_first_col:
        sta     $FFFF                   ; Patched

        sta     hn_val
        ; compute new branch offset (from high_nibble_special to high_nibble_end)
        lda     #<(high_nibble_end-high_nibble_special_neg-2)
        sta     high_nibble_special_neg+1
        lda     #<(high_nibble_end-high_nibble_special_pos-2)
        sta     high_nibble_special_pos+1

        lda     hn_val
        bra     do_low_nibble

; --------------------------------------; End of inlined helpers

col_outer_loop:                         ; Outer column loop, iterating 2 or 4 times
        ldy    #0

col_inner_loop:                         ; Inner column loop, iterating over Y
        lda     (cur_cache_ptr)
        inc     cur_cache_ptr
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

        clc
        adc     high_nibble_gstep_low,x ; Sets carry if overflow

        bcc     clamp_high_nibble_low   ; Clamp low as gstep is negative
        clc
high_nibble_special_neg:
        bra     handle_first_pixel      ; Patched for special cases (first_pixel, then first_row, then first_col or high_nibble_end)

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
        bra     handle_first_pixel      ; Patched for special cases (first_pixel, then first_row, then first_col or high_nibble_end)

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
        bra     handle_first_row_low    ; Patched (first_row, then low_nibble_end)

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

        dec     a
        sta     idx_one                 ; idx_one = idx-1
        bcc     :+
        inc     idx_one+1
        clc

:       tya                             ; Y = INNER_X_LOOP_LEN
        adc     idx_behind              ; idx_behind += INNER_X_LOOP_LEN
        sta     idx_behind
        bcc     :+
        inc     idx_behind+1
        clc

:       jmp     col_outer_loop

end_of_row:
        lda     ln_val                  ; *(idx+2) = val
        ldy     #2
        sta     (idx),y

first_row_special:                      ; Update special handlers at end of first row
        bra     finish_first_row        ; (will be patched out)


; --------------------------------------; Inlined helpers, close enough to branch

; Handle end of row
row_done:
        ; Put back first col handler for high nibble
        lda     #<(handle_first_col-high_nibble_special_neg-2)
        sta     high_nibble_special_neg+1
        lda     #<(handle_first_col-high_nibble_special_pos-2)
        sta     high_nibble_special_pos+1

        ldy     row
        dey
        beq     band_done
        sty     row
        jmp     row_loop

        ; First cols and first row handlers, out of main loop

; ------
; Clamp value to 8 bits
clamp_low_nibble:
        eor     #$FF                     ; => 00 if negative, FE if positive
        bpl     :+
        lda     #$FF                     ; => FF if positive

:       sta     ln_val
low_nibble_clamped:
        bra     handle_first_row_low

; ------
; Handle first row special case (low nibble)
handle_first_row_low:
        sta     (idx_behind),y           ; *(idx_behind+4) = *(idx_behind+6) = val;
        iny
        iny
        sta     (idx_behind),y
        dey
        dey
        jmp     low_nibble_end

        ; End of first col/row handlers

; ------
; Handle end of first row
finish_first_row:
        ;lda     #<(low_nibble_end-low_nibble_special-2) == 0
        stz     low_nibble_special+1

        lda     #<(low_nibble_end-low_nibble_clamped-2)
        sta     low_nibble_clamped+1

        stz     first_row_special+1     ; Unplug first row handler
        bra     row_done

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
        sta     (sp),y
        dey
        sta     (sp),y

        dey                              ; -1,
        sta     (sp),y
        dey
        sta     (sp),y

        dey                              ; 80*22,
        lda     #>(80*22)
        sta     (sp),y
        dey
        lda     #<(80*22)
        sta     (sp),y

        dey                              ; pgbar_state (long)
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

        stz     sreg+1                   ; height (long)
        stz     sreg
        lda     _height
        ldx     _height+1
        jmp     _progress_bar
