;
; Colin Leroy-Mira <colin@colino.net>, 2024
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.

; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; You should have received a copy of the GNU General Public License
; along with this program. If not, see <http://www.gnu.org/licenses/>.

        .export         _surl_stream_av
        .export         _SAMPLES_BASE

        .importzp       _zp6, _zp8, _zp9, _zp10, _zp12, _zp13, tmp1, tmp2, tmp3, ptr1, ptr2, ptr3, ptr4

        .import         _serial_putc_direct
        .import         _serial_read_byte_no_irq
        .import         _simple_serial_set_irq
        .import         _simple_serial_flush
        .import         _sleep, _init_text, _clrscr
        .import         _printer_slot, _data_slot

        .import         acia_status_reg_r, acia_data_reg_r

        .include        "apple2.inc"

; -----------------------------------------------------------------

MAX_LEVEL         = 31

.ifdef IIGS                   ; Todo
serial_status_reg = $00
serial_data_reg   = $00
.else
serial_status_reg = acia_status_reg_r
serial_data_reg   = acia_data_reg_r
.endif
HAS_BYTE          = $08

MAX_OFFSET    = 126
N_BASES       = (8192/MAX_OFFSET)+1
N_TEXT_BASES  = 4
PAGE_TOGGLE   = $7F

.ifdef DOUBLE_BUFFER
PAGE0_HB      = $20
PAGE1_HB      = $40
.else
PAGE0_HB      = $20
PAGE1_HB      = PAGE0_HB
.endif

SPKR         := $C030

spkr_ptr      = _zp6            ; word - Pointer to SPKR to access in 5 cycles
next_offset   = _zp8            ; byte - Where to write next video byte
cur_mix       = _zp9            ; byte - HGR MIX status (for subtitles toggling)

cur_base      = ptr1            ; word - Current HGR base to write to
page_ptr_high = ptr4            ; word - Pointer to bases addresses (high byte) array
zp_zero       = tmp1            ; byte - A zero in zero page (mostly to waste 3 cycles)
kbd_cmd       = tmp3            ; byte - Deferred keyboard command to handle

; ---------------------------------------------------------
;
; Macros

; ease cycle counting
.macro WASTE_2                  ; Cycles wasters
        nop
.endmacro

.macro WASTE_3
        stz     zp_zero
.endmacro

.macro WASTE_4
        nop
        nop
.endmacro

.macro WASTE_5
      WASTE_2
      WASTE_3
.endmacro

.macro WASTE_6
        nop
        nop
        nop
.endmacro

.macro WASTE_7
      WASTE_2
      WASTE_5
.endmacro

.macro WASTE_8
      WASTE_4
      WASTE_4
.endmacro

.macro WASTE_9
        WASTE_3
        WASTE_6
.endmacro

.macro WASTE_10
        WASTE_4
        WASTE_6
.endmacro

.macro WASTE_11
        WASTE_5
        WASTE_6
.endmacro

.macro WASTE_12
        WASTE_6
        WASTE_6
.endmacro

.macro WASTE_13
        WASTE_7
        WASTE_6
.endmacro

.macro WASTE_14
        WASTE_8
        WASTE_6
.endmacro

.macro WASTE_15
        WASTE_12
        WASTE_3
.endmacro

.macro WASTE_16
        WASTE_12
        WASTE_4
.endmacro

.macro WASTE_17
        WASTE_12
        WASTE_5
.endmacro

.macro WASTE_18
        WASTE_12
        WASTE_6
.endmacro

.macro WASTE_19
        WASTE_12
        WASTE_7
.endmacro

.macro WASTE_20
        WASTE_12
        WASTE_8
.endmacro

.macro WASTE_21
        WASTE_12
        WASTE_9
.endmacro

.macro WASTE_22
        WASTE_12
        WASTE_10
.endmacro

.macro WASTE_23
        WASTE_12
        WASTE_11
.endmacro

.macro WASTE_24
        WASTE_12
        WASTE_12
.endmacro

.macro WASTE_25
        WASTE_12
        WASTE_10
        WASTE_3
.endmacro

.macro WASTE_26
        WASTE_12
        WASTE_12
        WASTE_2
.endmacro

.macro WASTE_27
        WASTE_12
        WASTE_12
        WASTE_3
.endmacro

.macro WASTE_28
        WASTE_12
        WASTE_12
        WASTE_4
.endmacro

.macro WASTE_29
        WASTE_12
        WASTE_12
        WASTE_5
.endmacro

.macro WASTE_30
        WASTE_12
        WASTE_12
        WASTE_6
.endmacro

.macro WASTE_31
        WASTE_12
        WASTE_12
        WASTE_7
.endmacro

.macro WASTE_32
        WASTE_12
        WASTE_12
        WASTE_8
.endmacro

.macro WASTE_33
        WASTE_12
        WASTE_12
        WASTE_9
.endmacro

.macro WASTE_34
        WASTE_12
        WASTE_12
        WASTE_10
.endmacro

.macro WASTE_35
        WASTE_12
        WASTE_12
        WASTE_11
.endmacro

.macro WASTE_36
        WASTE_12
        WASTE_12
        WASTE_12
.endmacro

.macro WASTE_37
        WASTE_12
        WASTE_12
        WASTE_6
        WASTE_7
.endmacro

.macro WASTE_38
        WASTE_12
        WASTE_12
        WASTE_7
        WASTE_7
.endmacro

.macro WASTE_40
        WASTE_12
        WASTE_12
        WASTE_12
        WASTE_4
.endmacro

.macro WASTE_39
        WASTE_12
        WASTE_12
        WASTE_12
        WASTE_3
.endmacro

.macro WASTE_41
        WASTE_12
        WASTE_12
        WASTE_12
        WASTE_5
.endmacro

.macro WASTE_42
        WASTE_12
        WASTE_12
        WASTE_12
        WASTE_6
.endmacro

.macro WASTE_43
        WASTE_12
        WASTE_12
        WASTE_12
        WASTE_7
.endmacro

.macro WASTE_44
        WASTE_12
        WASTE_12
        WASTE_12
        WASTE_8
.endmacro

.macro WASTE_45
        WASTE_12
        WASTE_12
        WASTE_12
        WASTE_9
.endmacro

; Hack to waste 1 cycle. Use absolute stx with $00nn
.macro ABS_STX  zpvar
        .byte   $8E             ; stx absolute
        .byte   zpvar
        .byte   $00
.endmacro

; Hack to waste 1 cycle. Use absolute stz with $00nn
.macro ABS_STZ  zpvar
        .byte   $9C             ; stz absolute
        .byte   zpvar
        .byte   $00
.endmacro

; Hack to waste 1 cycle. Use absolute stz with $00nn
.macro ABS_STA  zpvar
        .byte   $8D             ; sta absolute
        .byte   zpvar
        .byte   $00
.endmacro

; Hack to waste 1 cycle. Use absolute sty with $00nn
.macro ABS_STY  zpvar
        .byte   $8C             ; sty absolute
        .byte   zpvar
        .byte   $00
.endmacro

; Hack to waste 1 cycle. Use absolute ldy with $00nn
.macro ABS_LDY  zpvar
        .byte   $AC             ; ldy absolute
        .byte   zpvar
        .byte   $00
.endmacro

; Hack to waste 1 cycle. Use absolute lda with $00nn
.macro ABS_LDA  zpvar
        .byte   $AD             ; lda absolute
        .byte   zpvar
        .byte   $00
.endmacro

.macro ____SPKR_DUTY____4       ; Toggle speaker
        sta     SPKR            ; 4
.endmacro

.ifdef __APPLE2ENH__
  .macro ____SPKR_DUTY____5       ; Toggle speaker slower (but without phantom-read)
          sta     (spkr_ptr)      ; 5
  .endmacro
.endif

.macro PREPARE_VIDEO_6
        nop                     ; 2
        tya                     ; 4      Get video byte in A
        cpy     #PAGE_TOGGLE    ; 6      Check for page toggle
.endmacro

.macro JUMP_NEXT_12
        WASTE_6                 ; 6
        jmp     (next,x)        ; 12     jump to next duty cycle
.endmacro

; General principles. we expect audio to be pushed on one serial port, video on
; the other. Execution flow  is controlled via the audio stream, with each
; received byte being the next duty cycle's address high byte.
;
; This means that duty cycle functions have to be page-aligned so their low byte
; is always 00.
; Functions that don't need to be aligned are stuffed between duty_cycle functions
; when possible, to waste less space.
;
; There are 32 different duty cycles (0-31), each moving the speaker by toggling
; it on and off at a certain cycles interval (from 8 to 39).
; The tasks of each duty cycle handler is:
; - Toggle the speaker on, wait a certain amount of cycles, toggle it off
; - Fetch the next byte of audio
; - Fetch the next byte of video
; - If there is a video byte, handle it
;
; It is possible to lose audio bytes without horrible effects (the sound quality
; would just be badder), but every video byte has to be fetched and handled,
; otherwise the video handler, being a state machine, will get messed up.
; Reading the same audio byte twice has no big drawback either, but reading the
; same video byte would, for the same reasons.
; In accordance to that, we spare ourselves the cycles required to verify the
; audio serial port's status register and unconditionnaly read the audio serial
; port's data register.
;
; Hence, each duty cycle does the following:
; - Load whatever is in the audio data register into jump destination
; - If there is a video byte,
;    - Load it,
;    - Finish driving speaker,
;    - Waste the required amount of cycles
;    - Jump to video handler
; - Otherwise,
;    - Load audio byte again,
;    - Waste the required amount of cycles (less than in the first case),
;    - Jump to the next duty cycle.
;
; Keyboard handling is cycle-expensive and can't be macroized properly, so
; reading the keyboard in cycle 15. This cycle, being in the middle, is,
; hopefully, called multiple hundreds of time per second.
;
; The video handler is responsible for jumping directly to the next duty
; cycle once the video byte is handled.
;
; Almost every reference to serial registers is direct, so every duty cycle
; is patched in multiple places. The patched instructions are labelled like
; ad0 (audio data cycle 0), and they need to be in *_patches arrays to be
; patched at start.
; Hardcoded placeholders have $xxFF where $xx is the register's address low
; byte for my setup, for reference.
; vs = video status = $99 (printer port in slot 1)
; vd = video data   = $98 (printer port in slot 1)
; as = audio status = $A9 (modem port in slot 2)
; ad = audio data   = $A8 (modem port in slot 2)

; To emulate 22kHz, we'd have to fit 29 duty cycles in 80 cycles,
; with jmp direct, from:
; toggle4-toggle4-other29-other3-toggle4-toggle4-other29-jump3
; to:
; toggle4-other29-toggle4-other3-toggle4-other29-toggle4-jump3
; This seems difficult to achieve (8 cycles needed for the second toggling,
; going from 68 cycles without to 80 with means no wasting at all)
;
; Warning about the alignment of the 32 duty cycles: as we read the next
; sample without verifying the ACIA's status register, we may read an
; incomplete byte, while it is being landed in the data register.
;
; So, we have to align the duty cycles in a way that even when this happens,
; we do not jump to a place where we have no duty cycle. This is why, here,
; we have them from $6000 to $7F00:
; $60 = 01100000
; $7F = 01111111
;
; At worst, we'll play a wrong sample from time to time. Tests with duty
; cycles from $6400 to $8300 crashed into the monitor quite fast:
; $64 = 01100100
; $83 = 10000011
; Reading an incomplete byte there could result in reading 11111111, for
; example, but not only. We don't want that.


        .segment        "AVMAGIC"

.align $40 ; page0_ and page1_ addresses arrays must share the same low byte
.assert * = $7C40, error                         ; We want this one's high byte to be even
PAGE0_ARRAY = *                                  ; for and'ing at video_sub:@set_offset
page0_addrs_arr_high:.res (N_BASES+4+1)          ; Also $7C & $55 = $54
pages_addrs_arr_low: .res (N_BASES+4+1)

; -----------------------------------------------------------------
; Sneak function in available hole
setup:
        ; Setup pointer access to SPKR
        lda     #<(SPKR)
        sta     spkr_ptr
        lda     #>(SPKR)
        sta     spkr_ptr+1

        ; Calculate bases for HGR page 0
        lda     #>(page0_addrs_arr_high)
        sta     calc_addr_high+2
        sta     calc_addr_text_high+2
        ldx     #PAGE0_HB
        jsr     calc_bases
        lda     #$50
        ldx     #$06
        jsr     calc_text_bases

        ; Calculate bases for HGR page 1
        lda     #>(page1_addrs_arr_high)
        sta     calc_addr_high+2
        sta     calc_addr_text_high+2
        ldx     #PAGE1_HB
        jsr     calc_bases
        lda     #$50
        ldx     #$0A
        jsr     calc_text_bases

        ; Init vars
        stz     kbd_cmd
        stz     cur_mix
        stz     next_offset

        ; Setup serial registers
        jsr     patch_serial_registers

acmd:   lda     $A8FF           ; Copy command and control registers from
vcmd:   sta     $98FF           ; the main serial port to the second serial
actrl:  lda     $A8FF           ; port, it's easier than setting it up from
vctrl:  sta     $98FF           ; scratch

        lda     #$2F            ; Surl client ready
        jsr     _serial_putc_direct

        jsr     _serial_read_byte_no_irq
        sta     enable_subs

        lda     #<(page0_addrs_arr_high)
        sta     page_ptr_high
        lda     #>(page0_addrs_arr_high)
        sta     page_ptr_high+1

        lda     #$40
        sta     cur_base+1
        stz     cur_base

        rts

.align $40
.assert * = $7D40, error                         ; We want high byte to be odd
PAGE1_ARRAY = *                                  ; for and'ing at video_sub:@set_offset
page1_addrs_arr_high:.res (N_BASES+4+1)          ; also $7F & $55 = $55

page_addrs_arr: .byte >(page1_addrs_arr_high)     ; Inverted because we write to page 1
                .byte >(page0_addrs_arr_high)     ; when page 0 is active, and vice-versa

enable_subs:    .byte $1

        .segment        "AVCODE"

.align $100
_SAMPLES_BASE = *
; -----------------------------------------------------------------
duty_cycle0:                    ; end spkr at 8
        ____SPKR_DUTY____4      ; 4      toggle speaker on
        ____SPKR_DUTY____4      ; 8      toggle speaker off
vs0:    lda     $99FF           ; 12     load video status register
ad0:    ldx     $A8FF           ; 16     load audio data register
        and     #HAS_BYTE       ; 18     check if video has byte
        beq     no_vid0         ; 20/21  branch accordingly
vd0:    ldy     $98FF           ; 24     load video data
        WASTE_9                 ; 33     waste extra cycles
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid0:                        ;        we had no video byte
ad0b:   ldx     $A8FF           ; 25     load audio data register again
        WASTE_31                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle1:                    ; end spkr at 9
        ____SPKR_DUTY____4      ; 4
        ____SPKR_DUTY____5      ; 9
ad1:    ldx     $A8FF           ; 13
vs1:    lda     $99FF           ; 17
        and     #HAS_BYTE       ; 19
        beq     no_vid1         ; 21/22
vd1:    ldy     $98FF           ; 25
        WASTE_8                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid1:
ad1b:   ldx     $A8FF           ; 26
        WASTE_30                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle2:                    ; end spkr at 10
        ____SPKR_DUTY____4      ; 4
        WASTE_2                 ; 6
        ____SPKR_DUTY____4      ; 10
ad2:    ldx     $A8FF           ; 14
vs2:    lda     $99FF           ; 18
        and     #HAS_BYTE       ; 20
        beq     no_vid2         ; 22/23
vd2:    ldy     $98FF           ; 26
        WASTE_7                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid2:
ad2b:   ldx     $A8FF           ; 27
        WASTE_29                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle3:                    ; end spkr at 11
        ____SPKR_DUTY____4      ; 4
        WASTE_3                 ; 7
        ____SPKR_DUTY____4      ; 11
ad3:    ldx     $A8FF           ; 15
vs3:    lda     $99FF           ; 19
        and     #HAS_BYTE       ; 21
        beq     no_vid3         ; 23/24
vd3:    ldy     $98FF           ; 27
        WASTE_6                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid3:
ad3b:   ldx     $A8FF           ; 28
        WASTE_28                ; 56
        JUMP_NEXT_12            ; 68

.align $100
.assert * = _SAMPLES_BASE + $100, error
; -----------------------------------------------------------------
duty_cycle4:                    ; end spkr at 12
        ____SPKR_DUTY____4      ; 4
ad4:    ldx     $A8FF           ; 8
        ____SPKR_DUTY____4      ; 12
vs4:    lda     $99FF           ; 16
        and     #HAS_BYTE       ; 18
        beq     no_vid4         ; 20/21
vd4:    ldy     $98FF           ; 24
        WASTE_9                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid4:
ad4b:   ldx     $A8FF           ; 25
        WASTE_31                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle5:                    ; end spkr at 13
        ____SPKR_DUTY____4      ; 4
ad5:    ldx     $A8FF           ; 8
        ____SPKR_DUTY____5      ; 13
vs5:    lda     $99FF           ; 17
        and     #HAS_BYTE       ; 19
        beq     no_vid5         ; 21/22
vd5:    ldy     $98FF           ; 25
        WASTE_8                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid5:
ad5b:   ldx     $A8FF           ; 26
        WASTE_30                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle6:                    ; end spkr at 14
        ____SPKR_DUTY____4      ; 4
ad6:    ldx     $A8FF           ; 8
        WASTE_2                 ; 10
        ____SPKR_DUTY____4      ; 14
vs6:    lda     $99FF           ; 18
        and     #HAS_BYTE       ; 20
        beq     no_vid6         ; 22/23
vd6:    ldy     $98FF           ; 26
        WASTE_7                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid6:
ad6b:   ldx     $A8FF           ; 27
        WASTE_29                ; 56
        JUMP_NEXT_12            ; 68

; --------------------------------------------------------------
duty_cycle7:                    ; end spkr at 15
        ____SPKR_DUTY____4      ; 4
ad7:    ldx     $A8FF           ; 8
        WASTE_3                 ; 11
        ____SPKR_DUTY____4      ; 15
vs7:    lda     $99FF           ; 19
        and     #HAS_BYTE       ; 21
        beq     no_vid7         ; 23/24
vd7:    ldy     $98FF           ; 27
        WASTE_6                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid7:
ad7b:   ldx     $A8FF           ; 28
        WASTE_28                ; 56
        JUMP_NEXT_12            ; 68

.align $100
.assert * = _SAMPLES_BASE + $200, error
; -----------------------------------------------------------------
duty_cycle8:                    ; end spkr at 16
        ____SPKR_DUTY____4      ; 4
ad8:    ldx     $A8FF           ; 8
vs8:    lda     $99FF           ; 12
        ____SPKR_DUTY____4      ; 16
        and     #HAS_BYTE       ; 18
        beq     no_vid8         ; 20/21
vd8:    ldy     $98FF           ; 24
        WASTE_9                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid8:
ad8b:   ldx     $A8FF           ; 25
        WASTE_31                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle9:                    ; end spkr at 17
        ____SPKR_DUTY____4      ; 4
ad9:    ldx     $A8FF           ; 8
vs9:    lda     $99FF           ; 12
        ____SPKR_DUTY____5      ; 17
        and     #HAS_BYTE       ; 19
        beq     no_vid9         ; 21/22
vd9:    ldy     $98FF           ; 25
        WASTE_8                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid9:
ad9b:   ldx     $A8FF           ; 26
        WASTE_30                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle10:                    ; end spkr at 18
        ____SPKR_DUTY____4      ; 4
ad10:   ldx     $A8FF           ; 8
vs10:   lda     $99FF           ; 12
        WASTE_2                 ; 14
        ____SPKR_DUTY____4      ; 18
        and     #HAS_BYTE       ; 20
        beq     no_vid10        ; 22/23
vd10:   ldy     $98FF           ; 26
        WASTE_7                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid10:
ad10b:  ldx     $A8FF           ; 27
        WASTE_29                ; 56
        JUMP_NEXT_12            ; 68


; -----------------------------------------------------------------
duty_cycle11:                    ; end spkr at 19
        ____SPKR_DUTY____4      ; 4
ad11:   ldx     $A8FF           ; 8
vs11:   lda     $99FF           ; 12
        WASTE_3                 ; 15
        ____SPKR_DUTY____4      ; 19
        and     #HAS_BYTE       ; 21
        beq     no_vid11        ; 23/24
vd11:   ldy     $98FF           ; 27
        WASTE_6                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid11:
ad11b:  ldx     $A8FF           ; 28
        WASTE_28                ; 56
        JUMP_NEXT_12            ; 68


.align $100
.assert * = _SAMPLES_BASE + $300, error
; -----------------------------------------------------------------
duty_cycle12:                    ; end spkr at 20
        ____SPKR_DUTY____4      ; 4
ad12:   ldx     $A8FF           ; 8
vs12:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        WASTE_2                 ; 16
        ____SPKR_DUTY____4      ; 20
        beq     no_vid12        ; 22/23
vd12:   ldy     $98FF           ; 26
        WASTE_7                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid12:
ad12b:  ldx     $A8FF           ; 27
        WASTE_29                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle13:                    ; end spkr at 21
        ____SPKR_DUTY____4      ; 4
ad13:   ldx     $A8FF           ; 8
vs13:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid13        ; 16/17
        ____SPKR_DUTY____5      ; 21
vd13:   ldy     $98FF           ; 25
        WASTE_8                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid13:
        ____SPKR_DUTY____4      ; 21
ad13b:  ldx     $A8FF           ; 25
        WASTE_31                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle14:                    ; end spkr at 22
        ____SPKR_DUTY____4      ; 4
ad14:   ldx     $A8FF           ; 8
vs14:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid14        ; 16/17
        WASTE_2                 ; 18
        ____SPKR_DUTY____4      ; 22
vd14:   ldy     $98FF           ; 26
        WASTE_7                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid14:
        ____SPKR_DUTY____5      ; 22
ad14b:  ldx     $A8FF           ; 26
        WASTE_30                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle15:                    ; end spkr at 23
        ____SPKR_DUTY____4      ; 4
ad15:   ldx     $A8FF           ; 8
vs15:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid15        ; 16/17
        WASTE_3                 ; 19
        ____SPKR_DUTY____4      ; 23
vd15:   ldy     $98FF           ; 27
        WASTE_6                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid15:
        WASTE_2                 ; 19
        ____SPKR_DUTY____4      ; 23
ad15b:  ldx     $A8FF           ; 27
        WASTE_4                 ; 31
        lda     kbd_cmd         ; 34     handle subtitles switch
        ldy     cur_mix         ; 37
        cmp     #$09            ; 39
        bne     not_tab         ; 41/42
        lda     $C052,y         ; 45     not BIT, to preserve V flag
        tya                     ; 47
        eor     #$01            ; 49
        sta     cur_mix         ; 52
        ABS_STZ kbd_cmd         ; 56
        JUMP_NEXT_12            ; 68

not_tab:
        WASTE_14                ; 56
        JUMP_NEXT_12            ; 68

.align $100
.assert * = _SAMPLES_BASE + $400, error
; -----------------------------------------------------------------
duty_cycle16:                    ; end spkr at 24
        ____SPKR_DUTY____4      ; 4
ad16:   ldx     $A8FF           ; 8
vs16:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid16        ; 16/17
vd16:   ldy     $98FF           ; 20
        ____SPKR_DUTY____4      ; 24
        WASTE_9                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid16:
        WASTE_3                 ; 20
        ____SPKR_DUTY____4      ; 24
asp:    lda     $FFFF           ; 28     check serial tx empty
        and     #$10            ; 30
        beq     noser           ; 32/33

        lda     KBD             ; 36     read keyboard
        bpl     nokbd           ; 38/39
        sta     KBDSTRB         ; 42     clear keystrobe
        and     #$7F            ; 44
adp:    sta     $FFFF           ; 48     send cmd
        cmp     #$1B            ; 50
        beq     out             ; 52/53  if escape, exit forcefully
        ABS_STA kbd_cmd         ; 56
        JUMP_NEXT_12            ; 68     jump to next duty cycle
nokbd:
ad16b:  ldx     $A8FF           ; 43
        WASTE_13                ; 56
        JUMP_NEXT_12            ; 68
noser:  
        WASTE_23                ; 56
        JUMP_NEXT_12            ; 68

out:    jmp     break_out

; -----------------------------------------------------------------
duty_cycle17:                    ; end spkr at 25
        ____SPKR_DUTY____4      ; 4
ad17:   ldx     $A8FF           ; 8
vs17:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid17        ; 16/17
vd17:   ldy     $98FF           ; 20
        ____SPKR_DUTY____5      ; 25
        WASTE_8                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid17:
ad17b:  ldx     $A8FF           ; 21
        ____SPKR_DUTY____4      ; 25
        WASTE_31                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle18:                    ; end spkr at 26
        ____SPKR_DUTY____4      ; 4
ad18:   ldx     $A8FF           ; 8
vs18:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid18        ; 16/17
vd18:   ldy     $98FF           ; 20
        WASTE_2                 ; 22
        ____SPKR_DUTY____4      ; 26
        WASTE_7                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid18:
ad18b:  ldx     $A8FF           ; 21
        ____SPKR_DUTY____5      ; 26
        WASTE_30                ; 56
        JUMP_NEXT_12            ; 68

.align $100
.assert * = _SAMPLES_BASE + $500, error
; -----------------------------------------------------------------
duty_cycle19:                    ; end spkr at 27
        ____SPKR_DUTY____4      ; 4
ad19:   ldx     $A8FF           ; 8
vs19:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid19        ; 16/17
vd19:   ldy     $98FF           ; 20
        WASTE_2                 ; 22
        ____SPKR_DUTY____5      ; 27
        WASTE_6                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid19:
ad19b:  ldx     $A8FF           ; 21
        WASTE_2                 ; 23
        ____SPKR_DUTY____4      ; 27
        WASTE_29                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle20:                    ; end spkr at 28
        ____SPKR_DUTY____4      ; 4
ad20:   ldx     $A8FF           ; 8
vs20:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid20        ; 16/17
vd20:   ldy     $98FF           ; 20
        WASTE_4                 ; 24
        ____SPKR_DUTY____4      ; 28
        WASTE_5                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid20:
ad20b:  ldx     $A8FF           ; 21
        WASTE_3                 ; 24
        ____SPKR_DUTY____4      ; 28
        WASTE_28                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle21:                    ; end spkr at 29
        ____SPKR_DUTY____4      ; 4
ad21:   ldx     $A8FF           ; 8
vs21:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid21        ; 16/17
vd21:   ldy     $98FF           ; 20
        WASTE_5                 ; 25
        ____SPKR_DUTY____4      ; 29
        WASTE_4                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid21:
ad21b:  ldx     $A8FF           ; 21
        WASTE_3                 ; 24
        ____SPKR_DUTY____5      ; 29
        WASTE_27                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle22:                    ; end spkr at 30
        ____SPKR_DUTY____4      ; 4
ad22:   ldx     $A8FF           ; 8
vs22:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid22        ; 16/17
vd22:   ldy     $98FF           ; 20
        WASTE_6                 ; 26
        ____SPKR_DUTY____4      ; 30
        WASTE_3                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid22:
ad22b:  ldx     $A8FF           ; 21
        WASTE_5                 ; 26
        ____SPKR_DUTY____4      ; 30
        WASTE_26                ; 56
        JUMP_NEXT_12            ; 68

.align $100
.assert * = _SAMPLES_BASE + $600, error
; -----------------------------------------------------------------
duty_cycle23:                    ; end spkr at 31
        ____SPKR_DUTY____4      ; 4
ad23:   ldx     $A8FF           ; 8
vs23:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid23        ; 16/17
vd23:   ldy     $98FF           ; 20
        WASTE_7                 ; 27
        ____SPKR_DUTY____4      ; 31
        WASTE_2                 ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid23:
ad23b:  ldx     $A8FF           ; 21
        WASTE_6                 ; 27
        ____SPKR_DUTY____4      ; 31
        WASTE_25                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle24:                    ; end spkr at 32
        ____SPKR_DUTY____4      ; 4
ad24:   ldx     $A8FF           ; 8
vs24:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid24        ; 16/17
vd24:   ldy     $98FF           ; 20
        WASTE_8                 ; 28
        ____SPKR_DUTY____4      ; 32
        WASTE_3                 ; 35
        tya                     ; 37
        cpy     #PAGE_TOGGLE    ; 39
        jmp     video_sub       ; 42=>68

no_vid24:
ad24b:  ldx     $A8FF           ; 21
        WASTE_7                 ; 28
        ____SPKR_DUTY____4      ; 32
        WASTE_24                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle25:                    ; end spkr at 33
        ____SPKR_DUTY____4      ; 4
ad25:   ldx     $A8FF           ; 8
vs25:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid25        ; 16/17
vd25:   ldy     $98FF           ; 20
        WASTE_9                 ; 29
        ____SPKR_DUTY____4      ; 33
        PREPARE_VIDEO_6         ; 39
        jmp     video_sub       ; 42=>68

no_vid25:
ad25b:  ldx     $A8FF           ; 21
        WASTE_8                 ; 29
        ____SPKR_DUTY____4      ; 33
        WASTE_23                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle26:                    ; end spkr at 34
        ____SPKR_DUTY____4      ; 4
ad26:   ldx     $A8FF           ; 8
vs26:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid26        ; 16/17
vd26:   ldy     $98FF           ; 20
        WASTE_4                 ; 24
        PREPARE_VIDEO_6         ; 30
        ____SPKR_DUTY____4      ; 34
        WASTE_5                 ; 39
        jmp     video_sub       ; 42=>68

no_vid26:
ad26b:  ldx     $A8FF           ; 21
        WASTE_9                 ; 30
        ____SPKR_DUTY____4      ; 34
        WASTE_22                ; 56
        JUMP_NEXT_12            ; 68

.align $100
.assert * = _SAMPLES_BASE + $700, error
; -----------------------------------------------------------------
duty_cycle27:                    ; end spkr at 35
        ____SPKR_DUTY____4      ; 4
ad27:   ldx     $A8FF           ; 8
vs27:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid27        ; 16/17
vd27:   ldy     $98FF           ; 20
        WASTE_5                 ; 25
        PREPARE_VIDEO_6         ; 31
        ____SPKR_DUTY____4      ; 35
        WASTE_4                 ; 39
        jmp     video_sub       ; 42=>68

no_vid27:
ad27b:  ldx     $A8FF           ; 21
        WASTE_10                 ; 31
        ____SPKR_DUTY____4      ; 35
        WASTE_21                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle28:                    ; end spkr at 36
        ____SPKR_DUTY____4      ; 4
ad28:   ldx     $A8FF           ; 8
vs28:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid28        ; 16/17
vd28:   ldy     $98FF           ; 20
        WASTE_6                 ; 26
        PREPARE_VIDEO_6         ; 32
        ____SPKR_DUTY____4      ; 36
        WASTE_3                 ; 39
        jmp     video_sub       ; 42=>68

no_vid28:
ad28b:  ldx     $A8FF           ; 21
        WASTE_11                ; 32
        ____SPKR_DUTY____4      ; 36
        WASTE_20                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
duty_cycle29:                    ; end spkr at 37
        ____SPKR_DUTY____4      ; 4
ad29:   ldx     $A8FF           ; 8
vs29:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid29        ; 16/17
vd29:   ldy     $98FF           ; 20
        WASTE_7                 ; 27
        PREPARE_VIDEO_6         ; 33
        ____SPKR_DUTY____4      ; 37
        WASTE_2                 ; 39
        jmp     video_sub       ; 42=>68

no_vid29:
ad29b:  ldx     $A8FF           ; 21
        WASTE_12                ; 33
        ____SPKR_DUTY____4      ; 37
        WASTE_19                ; 56
        JUMP_NEXT_12            ; 68

; Duty cycle 30 must toggle off speaker at cycle 38, but we would have to jump
; to video_sub at cycle 39, so this one uses different entry points to
; the video handler to fix this.
; -----------------------------------------------------------------
duty_cycle30:                    ; end spkr at 38
        ____SPKR_DUTY____4      ; 4
ad30:   ldx     $A8FF           ; 8
vs30:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid30        ; 16/17
vd30:   ldy     $98FF           ; 20
        WASTE_5                 ; 25
        PREPARE_VIDEO_6         ; 31
        jmp     video_spkr_sub  ; 34=>68

no_vid30:
ad30b:  ldx     $A8FF           ; 21
        WASTE_13                ; 34
        ____SPKR_DUTY____4      ; 38
        WASTE_18                ; 56
        JUMP_NEXT_12            ; 68

.align $100
.assert * = _SAMPLES_BASE + $800, error
; -----------------------------------------------------------------
duty_cycle31:                    ; end spkr at 39
        ____SPKR_DUTY____4      ; 4
ad31:   ldx     $A8FF           ; 8
vs31:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid31        ; 16/17
vd31:   ldy     $98FF           ; 20
        WASTE_9                 ; 29
        PREPARE_VIDEO_6         ; 35
        ____SPKR_DUTY____4      ; 39
        jmp     video_sub       ; 42=>68

no_vid31:
ad31b:  ldx     $A8FF           ; 21
        WASTE_14                ; 35
        ____SPKR_DUTY____4      ; 39
        WASTE_17                ; 56
        JUMP_NEXT_12            ; 68

; -----------------------------------------------------------------
; VIDEO HANDLER
video_spkr_sub:                 ; Alternate entry point for duty cycle 30
        ____SPKR_DUTY____4      ; 38
        WASTE_4                 ; 42

; Video handler expects the video byte in Y register, and the N flag set by
; its loading.
; Video handler must take 26 cycles on every code path.

video_sub:
        bcc     @maybe_control          ; 2/3   Is it a data byte?

@set_pixel:                             ;       It is a data byte
        beq     @toggle_page            ; 4/5   Result of cpy #$7F
        ldy     next_offset             ; 7     Load the offset to the start of the base
        sta     (cur_base),y            ; 13    Store data byte
        inc     next_offset             ; 18    and increment offset.
        clv                             ; 20    Reset the offset-received flag.
        jmp     (next,x)                ; 26    Done, go to next duty cycle

@maybe_control:
        bvc     @set_offset             ; 5/6   If V flag is set, this one is a base byte

@set_base:                              ;       This is a base byte
        lda     pages_addrs_arr_low,y   ; 9     Load base pointer low byte from base array
        sta     cur_base                ; 12    Store it to destination pointer low byte
        lda     (page_ptr_high),y       ; 17    Load base pointer high byte from base array
        sta     cur_base+1              ; 20    Store it to destination pointer high byte
        jmp     (next,x)                ; 26    Done, go to next duty cycle

@set_offset:                            ;       This is an offset byte
        sty     next_offset             ; 9     Store offset
        lda     page_ptr_high+1         ; 12    Update the page flag here, where we have time
        and     #$55                    ; 14    Use the fact that page1 array's high byte is odd, and page0's is even
        sta     @toggle_page+1          ; 18
        adc     #$30                    ; 20    $54/$55 + $30 => sets V flag
        jmp     (next,x)                ; 26    Done, go to next duty cycle

@toggle_page:                           ;       Page toggling
.ifdef DOUBLE_BUFFER
        lda     $C054                   ; 9     Activate page (patched by set_offset)
        lda     page_ptr_high+1         ; 12    Toggle written page
        eor     #>page1_addrs_arr_high ^ >page0_addrs_arr_high
                                        ; 14
        sta     page_ptr_high+1         ; 17    No time to update page flag,
        WASTE_3                         ; 20
        jmp     (next,x)                ; 26    We'll do it in @set_offset
.else
        WASTE_15                        ; 20
        jmp     (next,x)                ; 26
.endif

.assert * < _SAMPLES_BASE + $A00, error ; We don't want to cross page in the middle of video_sub

; -----------------------------------------------------------------
break_out:
        jsr     _clrscr
        jsr     _init_text
        lda     #$01
        ldx     #$00
        jsr     _sleep

        lda     #$02            ; Disable second port
vcmd2:  sta     $98FF

        plp                     ; Reenable all interrupts
        lda     #$01            ; Reenable serial interrupts and flush
        jsr     _simple_serial_set_irq
        jsr     _simple_serial_flush
        lda     #$2F            ; SURL_CLIENT_READY
        jmp     _serial_putc_direct

; -----------------------------------------------------------------
_surl_stream_av:                ; Entry point
        php
        sei                     ; Disable all interrupts

        pha

        lda     #$00            ; Disable serial interrupts
        jsr     _simple_serial_set_irq

        pla
        ; Setup pointers
        jsr     setup

        ; Clear pages
        bit     $C082
        lda     #$40
        sta     $E6
        jsr     $F3F2
        lda     #$20
        sta     $E6
        jsr     $F3F2
        bit     $C080

        lda     #$2F            ; Surl client ready
        jsr     _serial_putc_direct

        clv                     ; clear offset-received flag

ass:    lda     $A9FF           ; Wait for an audio byte
        and     #HAS_BYTE
        beq     ass
ads:    ldx     $A8FF
        jmp     (next,x)

; -----------------------------------------------------------------
calc_bases:
        ; Precalculate addresses inside pages, so we can easily jump
        ; from one to another without complicated computations. X
        ; contains the base page address's high byte on entry ($20 for
        ; page 0, $40 for page 1)
        ldy     #0              ; Y is the index - Start at base 0
        lda     #$00            ; A is the address's low byte
                                ; (and X the address's high byte)

        clc
calc_next_base:
        sta     pages_addrs_arr_low,y         ; Store AX
        pha
        txa
calc_addr_high:
        sta     page0_addrs_arr_high,y
        pla
        iny

        adc     #(MAX_OFFSET)   ; Compute next base
        bcc     :+
        inx
:       cpy     #(N_BASES)
        bcc     calc_next_base
        rts

; -----------------------------------------------------------------
calc_text_bases:
        ; Precalculate text lines 20-23 adresses, so we can easily jump
        ; from one to another without complicated computations. X
        ; contains line 20's base page address high byte on entry ($02 for
        ; page 0, $06 for page 1).
        ldy     #(N_BASES)    ; Y is the index - Start after HGR bases

        clc
calc_next_text_base:
        sta     pages_addrs_arr_low,y        ; Store AX
        pha
        txa
calc_addr_text_high:
        sta     page0_addrs_arr_high,y
        pla
        iny

        adc     #$80                         ; Compute next base
        bcc     :+
        inx
:       cpy     #(N_BASES+4+1)
        bcc     calc_next_text_base
        rts

; -----------------------------------------------------------------
patch_addresses:                ; Patch all registers in ptr1 array with A
        ldy     #$00            ; Start at beginning
        sta     tmp1            ; Save value
next_addr:
        clc
        lda     (ptr1),y
        adc     #1              ; Add one to patch after label
        sta     ptr2
        iny
        lda     (ptr1),y
        beq     done            ; If high byte is 0, we're done
        adc     #0
        sta     ptr2+1
        iny

        .ifdef STREAM_CHECK_ADDR_PATCH
        lda     (ptr2)          ; Debug to be sure
        cmp     #$FF
        beq     :+
        brk
:
        .endif

        lda     tmp1            ; Patch low byte
        sta     (ptr2)

        inc     ptr2            ; Patch high byte with base (in X)
        bne     :+
        inc     ptr2+1
:       txa
        sta     (ptr2)

        bra     next_addr
done:
        rts

; -----------------------------------------------------------------
patch_serial_registers:
        .ifdef IIGS
        brk                     ; Todo
        .else
        clc
        ldx     #$C0            ; Serial registers' high byte
        lda     #<(video_status_patches)
        sta     ptr1
        lda     #>(video_status_patches)
        sta     ptr1+1
        lda     _printer_slot
        asl
        asl
        asl
        asl
        adc     #$89            ; ACIA STATUS (in slot 0)
        jsr     patch_addresses

        lda     #<(video_data_patches)
        sta     ptr1
        lda     #>(video_data_patches)
        sta     ptr1+1
        lda     _printer_slot
        asl
        asl
        asl
        asl
        adc     #$88            ; ACIA DATA (in slot 0)
        pha
        jsr     patch_addresses
        pla
        clc
        adc     #2
        sta     vcmd+1
        stx     vcmd+2
        sta     vcmd2+1
        stx     vcmd2+2
        adc     #1
        sta     vctrl+1
        stx     vctrl+2

        lda     #<(audio_status_patches)
        sta     ptr1
        lda     #>(audio_status_patches)
        sta     ptr1+1
        lda     _data_slot
        asl
        asl
        asl
        asl
        adc     #$89
        jsr     patch_addresses

        lda     #<(audio_data_patches)
        sta     ptr1
        lda     #>(audio_data_patches)
        sta     ptr1+1
        lda     _data_slot
        asl
        asl
        asl
        asl
        adc     #$88
        pha
        jsr     patch_addresses
        pla
        clc
        adc     #2
        sta     acmd+1
        stx     acmd+2
        adc     #1
        sta     actrl+1
        stx     actrl+2
        rts
        .endif

; -----------------------------------------------------------------
video_status_patches:
                .word vs0
                .word vs1
                .word vs2
                .word vs3
                .word vs4
                .word vs5
                .word vs6
                .word vs7
                .word vs8
                .word vs9
                .word vs10
                .word vs11
                .word vs12
                .word vs13
                .word vs14
                .word vs15
                .word vs16
                .word vs17
                .word vs18
                .word vs19
                .word vs20
                .word vs21
                .word vs22
                .word vs23
                .word vs24
                .word vs25
                .word vs26
                .word vs27
                .word vs28
                .word vs29
                .word vs30
                .word vs31
                .word $0000

; -----------------------------------------------------------------
video_data_patches:
                .word vd0
                .word vd1
                .word vd2
                .word vd3
                .word vd4
                .word vd5
                .word vd6
                .word vd7
                .word vd8
                .word vd9
                .word vd10
                .word vd11
                .word vd12
                .word vd13
                .word vd14
                .word vd15
                .word vd16
                .word vd17
                .word vd18
                .word vd19
                .word vd20
                .word vd21
                .word vd22
                .word vd23
                .word vd24
                .word vd25
                .word vd26
                .word vd27
                .word vd28
                .word vd29
                .word vd30
                .word vd31
                .word $0000

; -----------------------------------------------------------------
audio_status_patches:
                .word ass
                .word asp
                .word $0000

; -----------------------------------------------------------------
audio_data_patches:
                .word ads
                .word adp
                .word ad0
                .word ad0b
                .word ad1
                .word ad1b
                .word ad2
                .word ad2b
                .word ad3
                .word ad3b
                .word ad4
                .word ad4b
                .word ad5
                .word ad5b
                .word ad6
                .word ad6b
                .word ad7
                .word ad7b
                .word ad8
                .word ad8b
                .word ad9
                .word ad9b
                .word ad10
                .word ad10b
                .word ad11
                .word ad11b
                .word ad12
                .word ad12b
                .word ad13
                .word ad13b
                .word ad14
                .word ad14b
                .word ad15
                .word ad15b
                .word ad16
                .word ad16b
                .word ad17
                .word ad17b
                .word ad18
                .word ad18b
                .word ad19
                .word ad19b
                .word ad20
                .word ad20b
                .word ad21
                .word ad21b
                .word ad22
                .word ad22b
                .word ad23
                .word ad23b
                .word ad24
                .word ad24b
                .word ad25
                .word ad25b
                .word ad26
                .word ad26b
                .word ad27
                .word ad27b
                .word ad28
                .word ad28b
                .word ad29
                .word ad29b
                .word ad30
                .word ad30b
                .word ad31
                .word ad31b
                .word $0000

; -----------------------------------------------------------------
next:
                .word duty_cycle0
                .word duty_cycle1
                .word duty_cycle2
                .word duty_cycle3
                .word duty_cycle4
                .word duty_cycle5
                .word duty_cycle6
                .word duty_cycle7
                .word duty_cycle8
                .word duty_cycle9
                .word duty_cycle10
                .word duty_cycle11
                .word duty_cycle12
                .word duty_cycle13
                .word duty_cycle14
                .word duty_cycle15
                .word duty_cycle16
                .word duty_cycle17
                .word duty_cycle18
                .word duty_cycle19
                .word duty_cycle20
                .word duty_cycle21
                .word duty_cycle22
                .word duty_cycle23
                .word duty_cycle24
                .word duty_cycle25
                .word duty_cycle26
                .word duty_cycle27
                .word duty_cycle28
                .word duty_cycle29
                .word duty_cycle30
                .word duty_cycle31
                .repeat 32          ; Make sure we cover every number from
                .word break_out     ; to 32 to 63, in case we catch the end
                .endrep             ; of stream byte mid-byte
