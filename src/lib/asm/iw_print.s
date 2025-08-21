        .export     _dhgr_to_iw, _hgr_to_iw

        .import     _hgr_baseaddr_h, _hgr_baseaddr_l

        .import     _serial_putc_direct
        .import     _simple_serial_write
        .import     _simple_serial_getc_immediate
        .import     _simple_serial_getc_with_timeout

        .import     _progress_bar, _scrw

        .import     _kbhit, _cgetc

        .import     pushax, subysp
        .importzp   _zp10p, sreg, c_sp

        .include    "apple2.inc"

line_ptr =  _zp10p

        .segment "DATA"

.proc setup_binary_print_cmd
        .byte $1B, 'n', $1B, "T16"
.endproc

.proc disable_auto_line_feed
        .byte $1B, 'Z', $80, $00
.endproc

.proc send_560_chars_cmd
        .byte $1B, 'G', "0560"
.endproc
.proc send_280_chars_cmd
        .byte $1B, 'G', "0280"
.endproc

        .segment "CODE"

.proc serial_crlf
        lda     #$0D
        jsr     _serial_putc_direct
        lda     #$0A
        jmp     _serial_putc_direct
.endproc

.proc _dhgr_to_iw
        lda     #$03
        sta     iw_ini_bits
        lda     #$FF
        sta     scale_line
        sta     is_dhgr
        lda     #$00
        sta     scale_row
        lda     #$04
        sta     inc_line
        lda     #80
        sta     bytes_per_line
        jmp     start_print
.endproc

; A: scale?
.proc _hgr_to_iw
        cmp     #$01
        beq     hgr_1x
hgr_2x:
        lda     #$03
        sta     iw_ini_bits
        lda     #$FF
        sta     scale_line
        sta     scale_row
        lda     #$00
        sta     is_dhgr
        lda     #$04
        sta     inc_line
        lda     #40
        sta     bytes_per_line
        jmp     start_print

hgr_1x:
        lda     #$01
        sta     iw_ini_bits
        lda     #$00
        sta     scale_line
        sta     is_dhgr
        lda     #$00
        sta     scale_row
        lda     #$08
        sta     inc_line
        lda     #40
        sta     bytes_per_line
        jmp     start_print
.endproc

.segment "LC"   ; This function is required to be in LC for reading AUXMEM

.proc start_print
        jsr     wait_imagewriter_ready
        beq     :+
        rts

:       lda     #<disable_auto_line_feed
        ldx     #>disable_auto_line_feed
        jsr     pushax
        lda     #<.sizeof(disable_auto_line_feed)
        ldx     #>.sizeof(disable_auto_line_feed)
        jsr     _simple_serial_write

        ; Blank lines for margin
        jsr     serial_crlf
        jsr     serial_crlf
        jsr     serial_crlf

        ; Clear 80COL so we can read AUX if needed.
        lda     RD80COL
        sta     orig_80state
        sta     CLR80COL

        ; Iterate over lines, by block of four or eight, depending on scale
        lda     #$00
next_four_lines:
        sta     cur_line

        ; Set binary mode
        lda     #<setup_binary_print_cmd
        ldx     #>setup_binary_print_cmd
        jsr     pushax
        lda     #<.sizeof(setup_binary_print_cmd)
        ldx     #>.sizeof(setup_binary_print_cmd)
        jsr     _simple_serial_write

        ; Tell printer how many chars for this line
        ldy     #<send_560_chars_cmd
        ldx     #>send_560_chars_cmd
        lda     is_dhgr
        ora     scale_row
        bne     :+
        ldy     #<send_280_chars_cmd
        ldx     #>send_280_chars_cmd
:       tya
        jsr     pushax
        lda     #<.sizeof(send_560_chars_cmd)
        ldx     #>.sizeof(send_560_chars_cmd)
        jsr     _simple_serial_write

        ; Here we iterate on blocks of 7 pixels by 4 or 8 lines,
        ; so that we can fetch the pixel in main and aux a bit
        ; more easily
        ldx     #$00          ; for x = 0, x < 40 or 80
next_hgr_byte:
        stx     cur_hgr_byte
        txa
        ldx     #$00

        bit     is_dhgr
        bpl     set_is_aux

        lsr     a             ; div cur_hgr_byte by two
        bcs     set_is_aux    ; was it odd?
        ldx     #$FF          ; Yes so we'll read in AUXMEM
set_is_aux:
        stx     is_aux
        tay                   ; send cur_hgr_byte to Y

        lda     #$01
        sta     hgr_mask
next_hgr_x:
        lda     #$00          ; Init a new byte to send to IW
        sta     print_byte
        lda     iw_ini_bits   ; Either 00000001 or 00000011, depending on Y scale
        sta     iw_bits

        ldx     cur_line
next_iw_byte:
        lda     _hgr_baseaddr_l,x
        sta     line_ptr
        lda     _hgr_baseaddr_h,x
        ; clc
        ; adc     is_aux
        sta     line_ptr+1

        bit     is_aux
        bpl     :+
        sta     $C003         ; RDCARDRAM - inefficient to switch for each byte, but...
:       lda     (line_ptr),y
        sta     $C002         ; RDMAINRAM - I don't want to move all vars to ZP or LC

        and     hgr_mask      ; Check if pixel is black
        bne     white_pixel
black_pixel:
        lda     iw_bits
        ora     print_byte
        sta     print_byte
white_pixel:
        inx
        bit     scale_line    ; Do we double Y?
        bpl     :+            ; If not, only shift by one so each IW byte has 8 rows
        asl     iw_bits
:       asl     iw_bits
        bne     next_iw_byte  ; If iw_bits is now 0, we did the 4 or 8 rows
        
        jsr     wait_imagewriter_ready
        bne     out

        lda     print_byte    ; Send this byte!
        jsr     _serial_putc_direct

        bit     scale_row     ; Should we send it twice for X doubling?
        bpl     :+
        jsr     _serial_putc_direct

:       asl     hgr_mask      ; on to next HGR dot
        bpl     next_hgr_x

        ldx     cur_hgr_byte  ; Did we finish that row's HGR bytes?
        inx
        cpx     bytes_per_line
        bcc     next_hgr_byte

        jsr     serial_crlf   ; Prepare for next line

        jsr     print_progress_bar

        jsr     _kbhit        ; Allow cancellation
        beq     :+
        jsr     _cgetc
        cmp     #$1B
        beq     out

:       lda     cur_line
        clc
        adc     inc_line      ; Increment cur line (by 4 or 8, depending on Y scaling)
        cmp     #192
        bcs     out           ; Do we have more lines?
        jmp     next_four_lines

out:
        lda     orig_80state  ; Restore 80col mode
        bpl     :+
        sta     SET80COL
:       rts
.endproc

.proc print_progress_bar
        lda     orig_80state
        bpl     :+
        sta     SET80COL

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

        dey                              ; scrw
        lda     #0
        sta     (c_sp),y
        dey
        lda     _scrw
        sta     (c_sp),y

        dey                              ; cur_line (long)
        lda     #0
        sta     (c_sp),y
        dey
        sta     (c_sp),y
        dey
        lda     #0
        sta     (c_sp),y
        dey
        lda     cur_line
        sta     (c_sp),y

        lda     #$00
        sta     sreg+1                   ; HGR_HEIGHT (long)
        sta     sreg
        lda     #192
        ldx     #0
        jsr     _progress_bar
        sta     CLR80COL
        rts
.endproc

.proc wait_imagewriter_ready
        lda    #$10
        sta    wait_xon

        jsr    _simple_serial_getc_immediate
        cmp    #$13           ; Did we get an XOFF?
        beq    wait_ready
ready:
        lda    #$00
        rts

wait_ready:
        jsr    _simple_serial_getc_with_timeout
        cmp    #$11           ; Did we get an XON?
        beq    ready
        dec    wait_xon
        bne    wait_ready
not_ready:
        lda    #$FF
        rts
.endproc

        .bss

cur_hgr_byte:   .res 1
is_aux:         .res 1
hgr_mask:       .res 1
print_byte:     .res 1
iw_ini_bits:    .res 1
iw_bits:        .res 1
cur_line:       .res 1

scale_row:      .res 1
scale_line:     .res 1
inc_line:       .res 1
is_dhgr:        .res 1
bytes_per_line: .res 1
orig_80state:   .res 1

wait_xon:       .res 1
