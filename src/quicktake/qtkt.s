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
SCRATCH_WIDTH = (640 + SCRATCH_PAD)
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
pgbar_state:
        .res        2,$00
motor_on:
        .res        2,$00

; Zero page
val               = _prev_rom_irq_vector
row               = _prev_rom_irq_vector+1
cur_cache_ptr     = _prev_ram_irq_vector

at_first_row      = _zp2
cur_row_loop      = _zp3
idx_one           = _zp4p
src               = _zp6p
idx               = _zp8p
idx_behind        = _zp10p
ln_val            = _zp12
hn_val            = _zp13


; Offset to scratch start of last scratch lines, row 20 col 0
LAST_TWO_LINES = _raw_image + (BAND_HEIGHT * SCRATCH_WIDTH)

.segment        "CODE"

set_cache_end:
        lda     _cache_end
        beq     :+
        brk                             ; Make sure cache end is aligned
:       lda     _cache_end+1            ; Patch end-of-cache comparison
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

; Don't put this one in LC as it is patched on runtime
_qt_load_raw:
        cmp     #$00
        bne     not_top
        cpx     #$00
        bne     not_top

top:    jsr     set_cache_end           ; Yes. Initialize things

        stz     pgbar_state

        lda     #(640/INNER_X_LOOP_LEN) ; How many outer loops per row ?
        ldx     _width
        cpx     #<(320)
        bne     :+
        lda     #(320/INNER_X_LOOP_LEN)
:       sta     set_row_loops+1         ; Patch outer col loop bound

        lda     #1
        sta     at_first_row

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

        lda     floppy_motor_on         ; Patch motor-on if we use a floppy
        beq     start_work
        sta     keep_motor_on+1
        sta     keep_motor_on_beg+1
        lda     #$C0
        sta     keep_motor_on+2
        sta     keep_motor_on_beg+2

        jmp     start_work

not_top:; Subsequent bands
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

        lda     #BAND_HEIGHT            ; We iterate over 20 lines
        sta     row

row_loop:                               ; Row loop
        clc
        lda     row                     ; Row & 1?
        bit     #$01
        beq     even_row

        ; Odd row
        lda     src                     ; Set idx_forward = src + SCRATCH_WIDTH
        tay
        adc     #<SCRATCH_WIDTH
        sta     store_idx_forward_first_col+1   ; No need to update store_idx_forward_first_pixel here
        lda     src+1
        tax
        adc     #>SCRATCH_WIDTH
        sta     store_idx_forward_first_col+2

        iny                             ; Finish with idx = src + 1
        bne     :+
        inx

:       sty     idx
        stx     idx+1

        jmp     row_work

even_row:
        ; Even row
        and     #$02            ; Row % 8?
        bne     :+
        jsr     update_progress_bar

:       clc
        lda     src                     ; Set idx_forward = src + SCRATCH_WIDTH + 1 and idx = src
        sta     idx
        tay
        adc     #<(SCRATCH_WIDTH+1)
        sta     store_idx_forward_first_pixel+1
        sta     store_idx_forward_first_col+1

        lda     src+1
        sta     idx+1
        tax
        adc     #>(SCRATCH_WIDTH+1)
        sta     store_idx_forward_first_pixel+2
        sta     store_idx_forward_first_col+2

row_work:
        ; We now have idx as a word in YX
        cpy     #$00
        bne     :+
        dex
:       dey
        sty     idx_one                 ; idx_one = idx-1
        stx     idx_one+1

set_row_loops:
        lda     #0                      ; Patched (# of outer loops per row)
        sta     cur_row_loop

        lda     (idx)                   ; Remember previous val before shifting
        sta     ln_val                  ; index

        lda     idx
        ldx     idx+1
        sta     store_idx_min2_first_pixel+1      ; Remember idx-2 for first pixel
        stx     store_idx_min2_first_pixel+2
        sta     store_idx_min2_first_col+1      ; Remember idx-2 for first columns
        stx     store_idx_min2_first_col+2

        sec                             ; Set idx_behind = idx - (SCRATCH_WIDTH-1)
        sbc     #<(SCRATCH_WIDTH-1)
        sta     idx_behind
        txa
        sbc     #>(SCRATCH_WIDTH-1)
        sta     idx_behind+1

        clc
        lda     #<SCRATCH_WIDTH         ; src += SCRATCH_WIDTH
        adc     src
        sta     src
        lda     #>SCRATCH_WIDTH
        adc     src+1
        sta     src+1

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
        bne     handle_byte
        phy                             ; Backup registers before loading from disk
        pha
        jsr     fill_cache
        pla
        ply
keep_motor_on:
        sta     motor_on                ; Keep drive motor running
        bra     handle_byte

; Clamp value to 8bits
clamp_high_nibble:
        eor     #$FF                    ; => 00 if negative, FE if positive
        bpl     :+
        lda     #$FF                    ; => FF if positive

:       sta     hn_val                  ; Update val
high_nibble_clamped:
        bra     handle_first_pixel      ; Back to main loop

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

        ; compute new branch offset (from high_nibble_special to first_row)
        lda     #<(handle_first_row_high-high_nibble_special-2)
        sta     high_nibble_special+1

        ; compute new branch offset (from high_nibble_clamped to first_row)
        lda     #<(handle_first_row_high-high_nibble_clamped-2)
        sta     high_nibble_clamped+1

        bra     do_low_nibble           ; Back to main loop

; Handle first row's special case, for high nibble
handle_first_row_high:
        sta     (idx_behind),y          ; *(idx_behind+2) = *(idx_behind+4) = val;
        iny
        iny
        sta     (idx_behind),y
        dey
        dey                             ; Set Y back for low nibble
        bra     high_nibble_end         ; Back to main loop

handle_first_col:
        sta     (idx_one),y
store_idx_forward_first_col:
        sta     $FFFF                   ; Patched
store_idx_min2_first_col:
        sta     $FFFF                   ; Patched

        ; compute new branch offset (from high_nibble_special to high_nibble_end)
        lda     #<(high_nibble_end-high_nibble_special-2)
        sta     high_nibble_special+1

        ; compute new branch offset (from high_nibble_special to high_nibble_end)
        lda     #<(high_nibble_end-high_nibble_clamped-2)
        sta     high_nibble_clamped+1
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

        clc
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

        sta     hn_val                  ; hn_val = val

        lda     high_nibble_gstep_high,x
                                        ; Carry set by previous adc if overflowed

        adc     #0                      ; If A = $FF and C set, C will stay set
        clc                             ; so clear it.
        bne     clamp_high_nibble       ; under/overflow - handle it
        lda     hn_val                  ; Otherwise reload hn_val

high_nibble_special:
        bra     handle_first_pixel      ; Patched (first_pixel, then first_row, then first_col or high_nibble_end)

high_nibble_end:
                                        ; Not first col:
        adc     ln_val                   ; *(idx+1) = (val + ln_val) >> 1;
        ror
        clc
        sta     (idx_one),y

        ; REPEAT WITH LOW NIBBLE (same, with +2 offsets on Y)

do_low_nibble:
        ; val = ((((*(idx_behind+2)
        ;         + hn_val) >> 1)
        ;         + *(idx_behind+4)) >> 1)
        ;         + gstep[low_nibble];

        lda     hn_val                  ; Reload hn_val
        sta     (idx),y                 ; store *(idx+2) = val, we didn't do it earlier
        adc     (idx_behind),y          ; Y expected to be 2 there
        ror
        clc

        iny
        iny
        adc     (idx_behind),y
        ror

        clc                             ; + gstep[h].
        adc     low_nibble_gstep_low,x
                                        ; Sets carry if overflow

        sta     ln_val

        lda     low_nibble_gstep_high,x ; Carry set by previous adc if overflowed

        adc     #0                      ; If A = $FF and C set, C will stay set
        clc                             ; so clear it.
        bne     clamp_low_nibble        ; Overflow - handle it
        lda     ln_val                  ; otherwise reload ln_val

low_nibble_special:
        bra     handle_first_row_low    ; Patched (first_row, then low_nibble_end)

low_nibble_end:                         ; We have ln_val in A, store it
        sta     (idx),y                 ; at idx
        adc     hn_val                  ; and handle idx+1
        ror                             ; (don't bother clc, carry cleared at start of loop)
        sta     (idx_one),y
        ; END LOW NIBBLE

check_col_inner_loop:
        cpy     #INNER_X_LOOP_LEN
        bne     col_inner_loop

shift_indexes:
        lda     #(INNER_X_LOOP_LEN-1)   ; idx_behind += INNER_X_LOOP_LEN
        adc     idx_behind              ; Adding INNER_X_LOOP_LEN-1 because
        sta     idx_behind              ; carry is set by the previous cpy
        bcc     :+                      ; as Y == INNER_X_LOOP_LEN
        inc     idx_behind+1
        clc

:       tya                             ; Y = INNER_X_LOOP_LEN here
        adc     idx_one                 ; idx_one += INNER_X_LOOP_LEN
        sta     idx_one
        bcc     :+
        inc     idx_one+1
        clc

:       tya
        adc     idx                     ; idx += INNER_X_LOOP_LEN
        sta     idx                     ; increment idx last for bound checking
        bcc     :+
        inc     idx+1

:       dec     cur_row_loop            ; Are we at end of line?
        bne     col_outer_loop

end_of_row:
        lda     ln_val                  ; *(idx+2) = val
        ldy     #2
        sta     (idx),y


        ; Unplug first row handler for low nibble
        lda     at_first_row
        beq     :+
        ;lda     #<(low_nibble_end-low_nibble_special-2) == 0
        stz     low_nibble_special+1

        lda     #<(low_nibble_end-low_nibble_clamped-2)
        sta     low_nibble_clamped+1

        stz     at_first_row
:

        ; Put back first col handler for high nibble
        lda     #<(handle_first_col-high_nibble_special-2)
        sta     high_nibble_special+1

        lda     #<(handle_first_col-high_nibble_clamped-2)
        sta     high_nibble_clamped+1

        dec     row
        beq     :+
        jmp     row_loop
:       rts

        ; First cols and first row handlers, out of main loop

clamp_low_nibble:
        eor     #$FF                     ; => 00 if negative, FE if positive
        bpl     :+
        lda     #$FF                     ; => FF if positive

:       sta     ln_val
low_nibble_clamped:
        bra     handle_first_row_low

handle_first_row_low:
        sta     (idx_behind),y           ; *(idx_behind+4) = *(idx_behind+6) = val;
        iny
        iny
        sta     (idx_behind),y
        dey
        dey
        jmp     low_nibble_end

        ; End of first col/row handlers

update_progress_bar:
        lda     pgbar_state             ; Update progress bar
        clc
        adc     #4
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
