        .importzp        c_sp, sreg
        .importzp        tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4
        .importzp        _prev_rom_irq_vector, _prev_ram_irq_vector
        .importzp        _zp8, _zp9, _zp10, _zp11, _zp12, _zp13

        .import          _memcpy, _memset
				.import          pushax, decsp4, subysp, _cputsxy
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
        .export          _histogram_low, _histogram_high
        .export          _orig_y_table_l, _orig_y_table_h
        .export          _orig_x_offset, _special_x_orig_offset

        .include         "fcntl.inc"

; Zero page pointers and variables

cur_cache_ptr     = _prev_ram_irq_vector ; Cache pointer, 2-bytes
row               = _zp8                 ; Current row, 1 byte
loop              = _zp9
loops             = _zp10                ; Used across bands
row_page_inc      = _zp11                ; Used across bands
ln_val            = _zp12                ; Used across bands
hn_val            = _zp13                ; Used across bands

; Defines

; QTKT algorithm relies on two pixels "borders" around the
; image to simplify bound checking.
SCRATCH_HEIGHT= (BAND_HEIGHT + 1)
RAW_IMAGE_SIZE= ((SCRATCH_HEIGHT-1) * RAW_WIDTH + 644)

.segment        "DATA"

_magic:
        .byte        $71,$6B,$74,$6B,$00
_model:
        .addr        model_str
_cache_start:
        .addr        _cache

_reading_str: .byte          "Reading     ", $0D, $0A, $00
_decoding_str:.byte          "Decoding    ", $0D, $0A, $00

.segment        "LC"
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

IDX        = _raw_image + RAW_WIDTH
IDX_BEHIND = (IDX-RAW_WIDTH+1)

.macro INC_HIGH_PAGES
        txa
        adc     I1+2
        sta     I1+2
        sta     I2+2
        sta     I3+2
        sta     I4+2
        sta     I5+2

        txa
        adc     IB1+2
        sta     IB1+2
        sta     IB2+2
        sta     IB3+2
        sta     IB4+2
        sta     IB5+2
        sta     IB6+2
.endmacro

.macro INC_FIRST_ROW_PAGES
        txa
        adc     IBFR1+2
        sta     IBFR1+2
        sta     IBFR2+2
        sta     IBFR3+2
        sta     IBFR4+2
.endmacro

.macro SET_HIGH_PAGES
        lda     #>IDX
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

; Defined here to avoid holes due to alignment
_histogram_low:         .res 256
_histogram_high:        .res 256
_orig_x_offset:         .res 256
_special_x_orig_offset: .res 256

; No need to align anymore
_orig_y_table_l:        .res BAND_HEIGHT
_orig_y_table_h:        .res BAND_HEIGHT

; Whether to keep floppy motor on (patched to softswitch in the code)
motor_on:
        .res        2

; How many full reads before end of data
full_reads:
        .res        1
last_read:
        .res        2

; Offset to scratch start of last scratch lines, row 20 col 0
LAST_LINE = _raw_image + (BAND_HEIGHT * RAW_WIDTH)

.macro SET_BRANCH val, label
        lda     val
        sta     label
.endmacro

.macro UPDATE_BRANCH from, to
        .assert (to-from-2) >= -128, error
        .assert (to-from-2) <= 127, error
        lda     #<(to-from-2)
        sta     from+1
.endmacro

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
        ldx     #0
        lda     #7
        jsr     pushax
        lda     #<_reading_str
        ldx     #>_reading_str
        jsr     _cputsxy

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
        dec     full_reads
        bne     :+
        lda     last_read
        ldx     last_read+1
:       jsr     _read

        ldx     #0
        lda     #7
        jsr     pushax
        lda     #<_decoding_str
        ldx     #>_decoding_str
        jmp     _cputsxy

; Main loop, decodes a band
_qt_load_raw:
        cmp     #$00                    ; Check if we're on the first band
        bne     not_top
        cpx     #$00
        bne     not_top

top:
        jsr     set_cache_data          ; Initialize cache things

        ; Compute how many full reads
        ; and the size of the last read
        ; lda     #((320*240/4)/CACHE_SIZE)
        ; sta     full_reads
        ; lda     #<(((320*240/4) .mod CACHE_SIZE)+1024)
        ; sta     last_read
        ; lda     #>(((320*240/4) .mod CACHE_SIZE)+1024)
        ; sta     last_read+1

        ldx     #80
        ldy     #2
        lda     _width                  ; How many outer loops per row ?
        cmp     #<640
        bne     :+
        ldx     #160
        ldy     #1
        lda     #((640*480/4)/CACHE_SIZE)
        sta     full_reads
        lda     #<(((640*480/4) .mod CACHE_SIZE))
        sta     last_read
        lda     #>(((640*480/4) .mod CACHE_SIZE))
        sta     last_read+1
        lda     #$F0          ; BEQ - allow cache refill
        sta     no_cache_read

:       stx     loops
        sty     row_page_inc

        ; Init the second line + 2 bytes of buffer with grey
        lda     #<(_raw_image)
        ldx     #>(_raw_image)
        jsr     pushax
        lda     #$80
        ldx     #$00
        jsr     pushax
        lda     #<(RAW_WIDTH + 1)
        ldx     #>(RAW_WIDTH + 1)
        jsr     _memset

        ldy     #BAND_HEIGHT            ; We iterate over 20 rows
        sty     row

        ; Activate high_nibble_special - bcc
        SET_BRANCH #$90, high_nibble_special
        SET_BRANCH #$90, low_nibble_special

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

        lda     #<(_raw_image)
        ldx     #>(_raw_image)
        jsr     pushax
        lda     #<(LAST_LINE)
        ldx     #>(LAST_LINE)
        jsr     pushax
        lda     #<(RAW_WIDTH + 1)
        ldx     #>(RAW_WIDTH + 1)
        jsr     _memcpy

        ldy     #BAND_HEIGHT            ; We iterate over 20 rows
        sty     row

row_loop:                               ; Row loop
next_ln_val:
        lda     #$80
        sta     ln_val                  ; index

        lda     I1+2                    ; Set first col pointers
        sta     IFC1+2
        sta     IFC2+2

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
        ;clc
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

no_cache_read:
        .assert CACHE_SIZE >= 19200, error ; the following trick wouldn't work otherwise
        bne     handle_byte

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

; ----------------------------------
first_row_handler_high:
IBFR1:  sta     IDX_BEHIND+2,y
IBFR2:  sta     IDX_BEHIND+4,y

first_pixel_handler:
        bcs     std_col_handler_high    ; Patched
        sta     IDX+1,y
        sta     IDX,y
        sta     next_ln_val+1
        SET_BRANCH #$90, first_pixel_handler
        jmp     do_low_nibble

first_col_handler:
IFC1:   sta     IDX+1,y
IFC2:   sta     IDX,y
        sta     next_ln_val+1
        SET_BRANCH #$B0, high_nibble_special
        jmp     do_low_nibble

first_row_handler_low:
IBFR3:  sta     IDX_BEHIND+6,y
IBFR4:  sta     IDX_BEHIND+4,y
        jmp     std_col_handler_low
; ----------------------------------

gstep_high_pos:
IB3:    lda     IDX_BEHIND,y
        ;clc
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

high_nibble_special:
        bcc     first_row_handler_high  ; patched

std_col_handler_high:
        adc     ln_val
        ror
        clc
I2:     sta     IDX+1,y

do_low_nibble:
        ; Low nibble
IB5:    lda     IDX_BEHIND+2,y
        ;clc
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

low_nibble_special:
        bcc     first_row_handler_low   ; Patched

std_col_handler_low:
        adc     hn_val
        ror
        clc
I4:     sta     IDX+3,y

        dec     loop
        beq     end_of_row

        tya                             ; Increment col
        ;clc
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
        ldx     #1
        clc
        INC_HIGH_PAGES
inc_first_row_handler:
        INC_FIRST_ROW_PAGES
        jmp     col_loop

end_of_row:
        lda     ln_val
I5:     sta     IDX+6,y

        ldx     row_page_inc
        INC_HIGH_PAGES

        ; Re-enable special handler for first_col
        SET_BRANCH #$90, high_nibble_special
        UPDATE_BRANCH high_nibble_special, first_col_handler

        dec     row
        beq     band_done

next_row_handler:
        bit     row_loop

        ; Deactivate first_row handler for low nibble
        SET_BRANCH #$B0, low_nibble_special
        ; Deactivate first row pointers update
        lda     #$4C
        sta     inc_first_row_handler
        sta     next_row_handler
        lda     #<col_loop
        sta     inc_first_row_handler+1
        lda     #>col_loop
        sta     inc_first_row_handler+2

        jmp     row_loop
band_done:
        rts
