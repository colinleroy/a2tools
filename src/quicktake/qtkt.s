        .importzp        c_sp, sreg
        .importzp        tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4
        .importzp        _prev_rom_irq_vector, _prev_ram_irq_vector
        .importzp        _zp2, _zp3, _zp4, _zp5, _zp6, _zp7, _zp12, _zp13

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
SCRATCH_WIDTH = 768
SCRATCH_HEIGHT= (BAND_HEIGHT + 2)
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

IDX        = _raw_image + (2*SCRATCH_WIDTH)
IDX_BEHIND = (IDX-SCRATCH_WIDTH+1)

.macro INC_HIGH_PAGES
        ldx     I0+2
        inx
        stx     I0+2
        stx     I1+2
        stx     I2+2
        stx     I3+2
        stx     I4+2
        stx     I5+2

        ldx     IB1+2
        inx
        stx     IB1+2
        stx     IB2+2
        stx     IB3+2
        stx     IB4+2
        stx     IB5+2
        stx     IB6+2

        ldx     IFC1+2
        inx
        stx     IFC1+2
        stx     IFC2+2
.endmacro

.macro INC_FIRST_ROW_PAGES
        ldx     IBFR1+2
        inx
        stx     IBFR1+2
        stx     IBFR2+2
        stx     IBFR3+2
        stx     IBFR4+2
.endmacro

.macro SET_HIGH_PAGES
        lda     #>IDX
        sta     I0+2
        sta     I1+2
        sta     I2+2
        sta     I3+2
        sta     I4+2
        sta     I5+2
        sta     IFC1+2
        sta     IFC2+2

        lda     #>IDX_BEHIND
        sta     IB1+2
        sta     IB2+2
        sta     IB3+2
        sta     IB4+2
        sta     IB5+2
        sta     IB6+2
        sta     IBFR1+2
        sta     IBFR2+2
        sta     IBFR3+2
        sta     IBFR4+2

        lda     #>(IDX+SCRATCH_WIDTH)
        sta     IFFC1+2
.endmacro

; QTKT file magic
model_str:
        .byte        $31,$30,$30,$00

.segment        "BSS"
.align 256
; Disk cache
_cache:
        .res        CACHE_SIZE

.align 256
; Destination buffer for the band (8bpp grayscale)
_raw_image:
        .res        RAW_IMAGE_SIZE

; No need to align anymore

; Status bar state, kept between bands
pgbar_state:
        .res        2

; Whether to keep floppy motor on (patched to softswitch in the code)
motor_on:
        .res        2

loops:  .res        1
first_row: .res     1

; Zero page pointers and variables

cur_cache_ptr     = _prev_ram_irq_vector ; Cache pointer, 2-bytes

row               = _zp2                 ; Current row, 1 byte

loop              = _zp4
first_col         = _zp6

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

        ldx     #80
        lda     _width                  ; How many outer loops per row ?
        cmp     #<640
        bne     :+
        ldx     #160
:       stx     loops

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

        ldy     #BAND_HEIGHT            ; We iterate over 20 rows
        sty     row

        lda     #$FF                    ; Set first row
        sta     first_row

        lda     floppy_motor_on         ; Patch motor_on if we use a floppy
        beq     row_loop
        sta     start_floppy_motor+1
        lda     #$C0                    ; Firmware access space
        sta     start_floppy_motor+2

        jmp     row_loop

not_top:                                ; Subsequent bands
                                        ; Shift the last band's last line, plus 2 pixels,
                                        ; to second line of the new band.

        SET_HIGH_PAGES

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
I0:     lda     IDX+0                   ; Remember previous value before shifting
        sta     ln_val                  ; index

        lda     #$FF                    ; Re-enable first col handler
        sta     first_col
        lda     I0+2                    ; Set first col pointers
        sta     IFC1+2
        sta     IFC2+2
        clc
        adc     #>SCRATCH_WIDTH
        sta     IFFC1+2

        lda     loops
        sta     loop

        ldy     #0                      ; Start offset for even rows

        lda     row
        and     #1
        beq     col_loop
        iny

col_loop:
cache_read = *+1
        lda     $FFFF
        inc     cache_read
        beq     inc_cache_high

handle_byte:
        tax

        bmi     gstep_high_pos
gstep_high_neg:
        ; High nibble
IB1:    lda     IDX_BEHIND,y
        clc
        adc     ln_val
        ror
        clc
IB2:    adc     IDX_BEHIND+2,y
        ror
        clc
        adc     high_nibble_gstep_low,x ; Sets carry if overflow
        bcc     clamp_high_nibble_low
        clc
        jmp     store_hn_val

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
; --------------------------------------; End of inlined helpers

clamp_high_nibble_low:
        lda     #$00
        jmp    store_hn_val

clamp_high_nibble_high:
        lda     #$FF
        clc
        jmp     store_hn_val

gstep_high_pos:
IB3:    lda     IDX_BEHIND,y
        clc
        adc     ln_val
        ror
        clc
IB4:    adc     IDX_BEHIND+2,y
        ror
        clc
        adc     high_nibble_gstep_low,x ; Sets carry if overflow
        bcs     clamp_high_nibble_high

store_hn_val:
        sta     hn_val
I1:     sta     IDX+2,y

first_row_handler_high:
        bit     first_row
        bpl     first_col_handler
IBFR1:  sta     IDX_BEHIND+2,y
IBFR2:  sta     IDX_BEHIND+4,y

first_col_handler:
        bit     first_col
        bpl     std_col_handler_high
IFC1:   sta     IDX+1,y
IFC2:   sta     IDX,y
IFFC1:  sta     IDX+SCRATCH_WIDTH
        lda     #$00
        sta     first_col
        jmp     do_low_nibble

std_col_handler_high:
        adc     ln_val
        ror
        clc
I2:     sta     IDX+1,y

do_low_nibble:
        ; Low nibble
IB5:    lda     IDX_BEHIND+2,y
        clc
        adc     hn_val
        ror
        clc
IB6:    adc     IDX_BEHIND+4,y
        ror
        clc
        adc     low_nibble_gstep_low,x ; Sets carry if overflow
        sta     ln_val
        lda     low_nibble_gstep_high,x ; Carry set by previous adc if overflowed
        adc     #0
        clc
        bne     clamp_low_nibble
        lda     ln_val
store_ln_val:
I3:     sta     IDX+4,y

first_row_handler_low:
        bit     first_row
        bpl     std_col_handler_low
IBFR3:   sta     IDX_BEHIND+6,y
IBFR4:   sta     IDX_BEHIND+4,y

std_col_handler_low:
        adc     hn_val
        ror
        clc
I4:     sta     IDX+3,y

        dec     loop
        beq     end_of_row

        tya                             ; Increment col
        clc
        adc     #4
        tay
        bcs     inc_idx_high
        jmp     col_loop

clamp_low_nibble:
        eor     #$FF                     ; => 00 if negative, FE if positive
        bpl     :+
        lda     #$FF                     ; => FF if positive
:       sta     ln_val
        jmp     store_ln_val

inc_idx_high:
        INC_HIGH_PAGES
        bit     first_row
        bpl     :+
        INC_FIRST_ROW_PAGES
:       jmp     col_loop

end_of_row:
        lda     ln_val
I5:     sta     IDX+6,y

        lda     #$00
        sta     first_row

        INC_HIGH_PAGES
        ldx     _width
        cpx     #<640
        beq     :+
        INC_HIGH_PAGES                  ; Double increment for 320 width images
:       dec     row

        beq     band_done
        jmp     row_loop

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
