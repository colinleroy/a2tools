        .export         _write_raw, _build_scale_table
        .export         _output_write_len
        .export         _scaled_band_height, _last_band, _last_band_crop
        .export         _scaling_factor
        .export         _effective_width
        .export         _crop_start_x, _crop_end_x
        .export         _crop_start_y, _crop_end_y

        .import         _histogram_low, _histogram_high

        .import         _write, _ofd, _reload_menu
        .import         _special_x_orig_offset, _orig_x_offset
        .import         _orig_y_table_l, _orig_y_table_h
        .import         _raw_image
        .import         _width

        .import         _cputs, _cgetc, _reload_menu
        .import         decsp4, pusha0, pushax, popax, incsp2
        .import         mulax10, tosudiva0, tosmulax

        .importzp       tmp1, _zp2, c_sp

y_ptr = _zp2

.proc _write_raw
        ldy     _last_band_crop   ; Is there cropping?
        beq     full_band

        cmp     _last_band        ; Are we on last band?
        bne     full_band
        cpx     _last_band+1
        bne     full_band

        lda     _last_band_crop   ; Register Y stop
        sta      y_end+1

        ; output_write_len -= (scaled_band_height - last_band_crop) * FILE_WIDTH;
        ; (FILE_WIDTH = 256 so shift 8)
        lda      _scaled_band_height
        sec
        sbc      _last_band_crop
        sta      tmp1
        sec
        lda      _output_write_len+1
        sbc      tmp1
        sta      _output_write_len+1
        jmp      no_crop

full_band:
        lda     _scaled_band_height
        sta     y_end+1

no_crop:
        lda     #>_raw_image
        sta     store_dest_pixel+2

        ldy     #$00
next_y:
        lda     _orig_y_table_l,y
        sta     load_source_pixel+1
        ldx     _orig_y_table_h,y
        stx     load_source_pixel+2

        iny
        sty     y_ptr

        ldy     #$00
        ldx     _orig_x_offset    ; Preload the first X offset, and
        jmp     load_source_pixel ; skip the first page increment

inc_orig_y_high:                  ; Out-of-band page increment of source data
        inc     load_source_pixel+2
        ldx     _special_x_orig_offset,y
        jmp     load_source_pixel

inc_histogram_high:               ; Out-of-band page increment of histogram
        inc     _histogram_high,x
        jmp     check_x

next_x:
        ldx     _orig_x_offset,y  ; if (*cur_orig_x == 0) cur+=256
        beq     inc_orig_y_high

load_source_pixel:
        lda     $FFFF,x           ; patched, get current pixel
store_dest_pixel:
        sta     _raw_image,y      ; Store scaled

        tax                       ; update histogram
        inc     _histogram_low,x
        beq     inc_histogram_high

check_x:
        iny
        bne     next_x
        inc     store_dest_pixel+2; Next output page

        ldy     y_ptr
y_end:  cpy     #$FF              ; Patched
        bcc     next_y

        jsr     decsp4            ; Call write
        ldy     #$03
        lda     #$00              ; ofd is never going to be > 255
        sta     (c_sp),y
        dey
        lda     _ofd
        sta     (c_sp),y
        dey
        lda     #>_raw_image
        sta     (c_sp),y
        dey
        lda     #<_raw_image
        sta     (c_sp),y
        lda     _output_write_len
        ldx     _output_write_len+1
        jmp     _write
.endproc

.segment "LC"

.proc _build_scale_table
        jsr     pushax            ; Backup ofname

        lda     _width            ; Divide crop boundaries if 320x240
        cmp     #<320
        bne     :+
        lsr     _crop_start_x+1
        ror     _crop_start_x
        lsr     _crop_start_y+1
        ror     _crop_start_y
        lsr     _crop_end_x+1
        ror     _crop_end_x
        lsr     _crop_end_y+1
        ror     _crop_end_y

:       sec
        lda     _crop_end_x       ; Compute effective_width
        sbc     _crop_start_x
        sta     _effective_width
        tax                       ; Remember low byte
        lda     _crop_end_x+1
        sbc     _crop_start_x+1
        sta     _effective_width+1

        cpx     #<640             ; Compare low byte for 640/320
        beq     scale_640
        cpx     #<320
        beq     scale_320
        cpx     #$00              ; Now if low byte != 0 we're wrong
        bne     default
        cmp     #>512             ; And high byte for 512/256
        beq     scale_512         ; As low == 0
        cmp     #>256
        beq     scale_256
default:
        lda     #<unsup_width_str
        ldx     #>unsup_width_str
        jsr     _cputs
        jsr     _cgetc
        jsr     popax
        jsr     _reload_menu
        brk
scale_320:
        lda     #8
        sta     _scaling_factor
        lda     #(BAND_HEIGHT*8/10)
        sta     _scaled_band_height

        lda     _width
        cmp     #<640
        bne     :+
        inc     _effective_width

:       lda     #<(FILE_WIDTH*BAND_HEIGHT*8/10)
        ldx     #>(FILE_WIDTH*BAND_HEIGHT*8/10)
        jmp     finish_scale
scale_640:
        lda     #4
        sta     _scaling_factor
        lda     #(BAND_HEIGHT*4/10)
        sta     _scaled_band_height
        lda     #<(FILE_WIDTH*BAND_HEIGHT*4/10)
        ldx     #>(FILE_WIDTH*BAND_HEIGHT*4/10)
        jmp     finish_scale
scale_512:
        lda     #5
        sta     _scaling_factor
        lda     #(BAND_HEIGHT*5/10)
        sta     _scaled_band_height

        lda     _crop_start_y
        clc
        adc     #<380
        sta     _last_band
        lda     _crop_start_y+1
        adc     #>380
        sta     _last_band+1
        lda     #2
        sta     _last_band_crop

        lda     #<(FILE_WIDTH*BAND_HEIGHT*5/10)
        ldx     #>(FILE_WIDTH*BAND_HEIGHT*5/10)
        jmp     finish_scale
scale_256:
        lda     #10
        sta     _scaling_factor
        lda     #(BAND_HEIGHT*10/10)
        sta     _scaled_band_height

        lda     _crop_start_y
        clc
        adc     #<180
        sta     _last_band
        lda     _crop_start_y+1
        adc     #>180
        sta     _last_band+1
        lda     #12
        sta     _last_band_crop

        lda     #<(FILE_WIDTH*BAND_HEIGHT*10/10)
        ldx     #>(FILE_WIDTH*BAND_HEIGHT*10/10)

finish_scale:
        sta     _output_write_len
        stx     _output_write_len+1

        ; Write-enable LC, some vars can be put there.
        bit     $C083
        bit     $C083

        ; Compute column scaling table
        ldy     #$00
        sty     prev_xoff_h
next_col:
        sty     col
        tya
        ldx     #$00              ; col
        jsr     mulax10           ; * 10
        jsr     pushax
        lda     _scaling_factor   ; / scaling_factor
        jsr     tosudiva0
        clc
        adc     #RAW_X_OFFSET
        bcc     :+
        inx

:       cpx     prev_xoff_h       ; Insert special offset for page change
        beq     :+

        stx     prev_xoff_h       ; update prev xoff high
        tax                       ; Backup xoff low
        ldy     col
        lda     #$00
        sta     _orig_x_offset,y
        txa                       ; Get xoff low back
        sta     _special_x_orig_offset,y
        jmp     advance_col

:       ldy     col
        sta     _orig_x_offset,y

advance_col:
        iny                       ; col++
        bne     next_col

        ; Compute lines scaling pointers
        ldy     _scaled_band_height
        dey
next_row:
        sty     row

        tya                       ; row
        ldx     #$00
        jsr     mulax10           ; * 10
        jsr     pushax
        lda     _scaling_factor   ; / scaling_factor
        jsr     tosudiva0
        jsr     pushax
        lda     #<RAW_WIDTH       ; * RAW_WIDTH
        ldx     #>RAW_WIDTH
        jsr     tosmulax
        clc
        adc     #<_raw_image      ; + raw_image
        tay
        txa
        adc     #>_raw_image
        tax

        tya                       ; + crop_start_x
        clc
        adc     _crop_start_x
        tay
        txa
        adc     _crop_start_x+1
        tax

        tya                       ; + RAW_Y_OFFSET*RAW_WIDTH
        clc
        adc     #<(RAW_Y_OFFSET*RAW_WIDTH)
        ldy     row
        sta     _orig_y_table_l,y
        txa
        adc     #>(RAW_Y_OFFSET*RAW_WIDTH)
        sta     _orig_y_table_h,y

        dey
        bpl     next_row
        jmp     incsp2            ; Drop ofname and return
.endproc

.segment "BSS"
col:                    .res 1
row:                    .res 1
prev_xoff_h:            .res 1
_scaled_band_height:    .res 1
_last_band:             .res 2
_last_band_crop:        .res 1
_scaling_factor:        .res 1
_effective_width:       .res 2
_crop_start_x:          .res 2
_crop_end_x:            .res 2
_crop_start_y:          .res 2
_crop_end_y:            .res 2
_output_write_len:      .res 2

.segment "DATA"
unsup_width_str:        .byte "Unsupported width.",$0D,$0A,$00
