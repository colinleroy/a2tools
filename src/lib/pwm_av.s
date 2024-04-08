;
; Colin Leroy-Mira, 2024
;
        .export         _pwm
        .export         _SAMPLES_BASE

        .importzp       _zp6, _zp8, _zp9, _zp10, _zp12, _zp13, tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4

        .import         _serial_putc_direct
        .import         _simple_serial_set_irq
        .import         _simple_serial_flush
        .import         popa, VTABZ
        .import         _printer_slot, _data_slot

        .import         acia_status_reg_r, acia_data_reg_r

        .include        "apple2.inc"

; ------------------------------------------------------------------------

MAX_LEVEL         = 31

serial_status_reg = acia_status_reg_r
serial_data_reg   = acia_data_reg_r
HAS_BYTE          = $08

MAX_OFFSET    = 126
N_BASES       = (8192/MAX_OFFSET)+1

.ifdef DOUBLE_BUFFER
PAGE1_HB      = $20
PAGE2_HB      = $40
.else
PAGE1_HB      = $20
PAGE2_HB      = PAGE1_HB
.endif

SPKR         := $C030

spkr_ptr      = _zp6
last_offset   = _zp8
got_offset    = _zp9
next          = _zp10
page          = _zp12
has_byte_zp   = _zp13
audio_status  = ptr1
audio_data    = ptr2
store_dest    = tmp1 ; + tmp2
dummy_zp      = tmp3
last_base     = tmp4
page1_addr_ptr= ptr3
page2_addr_ptr= ptr4

; ---------------------------------------------------------
;
; Macros to ease cycle counting

.macro WASTE_2                  ; Cycles wasters
        nop
.endmacro

.macro WASTE_3
        sta     dummy_zp
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

.macro WASTE_45   ; 18 nop + sta zp + 3 nop = 21+2 = 23 bytes
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

.macro KBD_LOAD_13              ; Check keyboard and jsr if key pressed (trashes A)
        lda     KBD             ; 4
        sta     KBDSTRB         ; 8
        cmp     #($1B|$80)      ; 10 - Escape?
        bne     :+              ; 12/13
        and     #$7F            ;
        jsr     _serial_putc_direct
        jmp     break_out
:

.endmacro

.macro ____SPKR_DUTY____4       ; Toggle speaker
        sta     SPKR            ; 4
.endmacro

.ifdef __APPLE2ENH__
  .macro ____SPKR_DUTY____5       ; Toggle speaker slower (but without phantom-read)
          sta     (spkr_ptr)      ; 5
  .endmacro
.endif

        .code

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
; Each duty cycle takes 83 cycles to complete (just below the 86µs interval at
; which bytes arrive on a serial port at 115200 bps), but as precisely timing
; the speaker toggling moves the serial fetching code around, it is possible to
; lose bytes of video if we only check once per duty cycle.
;
; Hence, each duty cycle does the following:
; - Load whatever is in the audio data register into jump destination
; - If there is a video byte,
;    - Load it,
;    - Finish driving speaker,
;    - Waste the required amount of cycles
;    - Jump to video handler
; - Otherwise,
;    - Waste the required amount of cycles (less than in the first case),
;    - Check video byte again,
;    - If there is one, jump to video handler,
; - Otherwise,
;    - Handle keyboard input,
;    - Load audio byte again,
;    - Waste (a lot) of cycles,
;    - Jump to the next duty cycle.
;
; The video handler has one code path where cycles are wasted, in which
; the next audio byte is loaded again so we lose less. It is responsible
; for jumping directly to the next duty cycle once the video byte is handled.
;
; As a rule of thumb, no bytes are dropped if we check for video byte around
; cycles 12-20 and 24-31.
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

.align 256
_SAMPLES_BASE = *
.assert * = $6000, error
duty_cycle0:                    ; end spkr at 8
        ____SPKR_DUTY____4      ; 4      toggle speaker on
        ____SPKR_DUTY____4      ; 8      toggle speaker off
vs0:    lda     $99FF           ; 12     load video status register
ad0:    ldx     $A8FF           ; 16     load audio data register
        and     #HAS_BYTE       ; 18     check if video has byte
        beq     no_vid0         ; 20/21  branch accordingly
vd0:    lda     $98FF           ; 24     load video data
        stx     next+1          ; 27     store next duty cycle destination
        WASTE_12                ; 39     waste extra cycles
        jmp     video_direct    ; 42=>83 handle video byte

no_vid0:                        ;        we had no video byte first try
        stx     next+1          ; 24     store next duty cycle destination
        WASTE_3                 ; 27     waste extra cycles
vs0b:   lda     $99FF           ; 31     check video status register again
        and     #HAS_BYTE       ; 33     do we have one?
        beq     no_vid0b        ; 35/36  branch accordingly
vd0b:   lda     $98FF           ; 39     load video data
        jmp     video_direct    ; 42=>83 handle video byte

no_vid0b:                       ;        we had no video byte second try
        WASTE_3                 ; 39     waste cycles
ad0b:   ldx     $A8FF           ; 43     load audio data register again
        stx     next+1          ; 46     store next duty cycle destination
        KBD_LOAD_13             ; 59     handle keyboard
        WASTE_18                ; 77     waste extra cycles
        jmp     (next)          ; 83     jump to next duty cycle

; -----------------------------------------------------------------
_pwm:                           ; Entry point
        pha
        ; Disable interrupts
        lda     #$00
        jsr     _simple_serial_set_irq

        pla
        ; Setup pointers
        jsr     setup_pointers

        lda     #$2F            ; Surl client ready
        jsr     _serial_putc_direct

as31:   lda     $A9FF           ; Wait for first byte,
        and     #HAS_BYTE
        beq     as31
        jmp     duty_cycle31    ; And start!
; -----------------------------------------------------------------
setup_pointers:
        ; Setup pointer access to SPKR
        lda     #<(SPKR)
        sta     spkr_ptr
        lda     #>(SPKR)
        sta     spkr_ptr+1

        ; Calculate bases for HGR page 1
        lda     #<(page1_addrs_arr)
        ldy     #>(page1_addrs_arr)
        sta     page1_addr_ptr
        sty     page1_addr_ptr+1
        ldx     #PAGE1_HB
        jsr     calc_bases

        ; Calculate bases for HGR page 2
        lda     #<(page2_addrs_arr)
        ldy     #>(page2_addrs_arr)
        sta     page2_addr_ptr
        sty     page2_addr_ptr+1
        ldx     #PAGE2_HB
        jsr     calc_bases

        ; Init cycle destination
        lda     #<(duty_cycle31)
        sta     next
        lda     #>(duty_cycle31)
        sta     next+1

        ; Setup serial registers
        jsr     patch_serial_registers

        lda     serial_status_reg+1
        sta     audio_status
        lda     serial_status_reg+2
        sta     audio_status+1

        lda     serial_data_reg+1
        sta     audio_data
        lda     serial_data_reg+2
        sta     audio_data+1

acmd:   lda     $A8FF           ; Copy command and control registers from
vcmd:   sta     $98FF           ; the main serial port to the second serial
actrl:  lda     $A8FF           ; port, it's easier than setting it up from
vctrl:  sta     $98FF           ; scratch

        ; Extra ZP variable to be able to waste one cycle using CMP ZP instead
        ; of CMP IMM in some duty cycles
        lda     #HAS_BYTE
        sta     has_byte_zp
        rts
; -----------------------------------------------------------------
calc_bases:
        ; Precalculate addresses inside pages, so we can easily jump
        ; from one to another without complicated computations. X
        ; contains the base page address's high byte on entry ($20 for
        ; page 1, $40 for page 2)
        sta     ptr1
        sty     ptr1+1

        ldy     #0              ; Y is the index - Start at base 0
        lda     #$00            ; A is the address's low byte
                                ; (and X the address's high byte)

        clc
calc_next_base:
        sta     (ptr1),y        ; Store AX
        iny
        pha
        txa
        sta     (ptr1),y
        pla
        iny

        adc     #(MAX_OFFSET)   ; Compute next base
        bcc     :+
        inx
        clc
:       cpy     #(N_BASES*2)
        bcc     calc_next_base
        rts
; -----------------------------------------------------------------

.align 256
.assert * = _SAMPLES_BASE + $100, error
duty_cycle1:                    ; end spkr at 9
        ____SPKR_DUTY____4      ; 4
        ____SPKR_DUTY____5      ; 9
ad1:    ldx     $A8FF           ; 13
vs1:    lda     $99FF           ; 17
        and     #HAS_BYTE       ; 19
        beq     no_vid1         ; 21/22
vd1:    lda     $98FF           ; 25
        stx     next+1          ; 28
        WASTE_11                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid1:
        WASTE_2                 ; 24
        stx     next+1          ; 27
vs1b:   lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid1b        ; 35/36
vd1b:   lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid1b:
ad1b:   ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

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

        lda     (ptr2)          ; Debug to be sure
        cmp     #$FF
        beq     :+
        brk
:

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

.align 256
.assert * = _SAMPLES_BASE + $200, error
duty_cycle2:                    ; end spkr at 10
        ____SPKR_DUTY____4      ; 4
        WASTE_2                 ; 6
        ____SPKR_DUTY____4      ; 10
ad2:    ldx     $A8FF           ; 14
vs2:    lda     $99FF           ; 18
        and     #HAS_BYTE       ; 20
        beq     no_vid2         ; 22/23
vd2:    lda     $98FF           ; 26
        stx     next+1          ; 29
        WASTE_10                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid2:
        ABS_STX next+1          ; 27 stx absolute
vs2b:   lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid2b        ; 35/36
vd2b:   lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid2b:
ad2b:   ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

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
        sta     tmp2
        jsr     patch_addresses
        lda     tmp2
        clc
        adc     #2
        sta     vcmd+1
        stx     vcmd+2
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
        sta     tmp2
        jsr     patch_addresses
        lda     tmp2
        clc
        adc     #2
        sta     acmd+1
        stx     acmd+2
        adc     #1
        sta     actrl+1
        stx     actrl+2
        rts
        .endif

.align 256
.assert * = _SAMPLES_BASE + $300, error
duty_cycle3:                    ; end spkr at 11
        ____SPKR_DUTY____4      ; 4
        WASTE_3                 ; 7
        ____SPKR_DUTY____4      ; 11
ad3:    ldx     $A8FF           ; 15
vs3:    lda     $99FF           ; 19
        and     #HAS_BYTE       ; 21
        beq     no_vid3         ; 23/24
vd3:    lda     $98FF           ; 27
        stx     next+1          ; 30
        WASTE_9                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid3:
        stx   next+1            ; 27
vs3b:   lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid3b        ; 35/36
vd3b:   lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid3b:
ad3b:   ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

video_status_patches:
                .word vs0
                .word vs0b
                .word vs1
                .word vs1b
                .word vs2
                .word vs2b
                .word vs3
                .word vs3b
                .word vs4
                .word vs4b
                .word vs5
                .word vs5b
                .word vs6
                .word vs6b
                .word vs7
                .word vs7b
                .word vs8
                .word vs8b
                .word vs9
                .word vs9b
                .word vs10
                .word vs10b
                .word vs11
                .word vs11b
                .word vs12
                .word vs12b
                .word vs13
                .word vs13b
                .word vs14
                .word vs14b
                .word vs15
                .word vs15b
                .word vs16
                .word vs16b
                .word vs17
                .word vs17b
                .word vs18
                .word vs18b
                .word vs19
                .word vs19b
                .word vs20
                .word vs20b
                .word vs21
                .word vs21b
                .word vs22
                .word vs22b
                .word vs23
                .word vs23b
                .word vs24
                .word vs24b
                .word vs25
                .word vs25b
                .word vs26
                .word vs26b
                .word vs27
                .word vs27b
                .word vs28
                .word vs28b
                .word vs29
                .word vs29b
                .word vs30
                .word vs30b
                .word vs31
                .word vs31b
                .word $0000

.align 256
.assert * = _SAMPLES_BASE + $400, error
duty_cycle4:                    ; end spkr at 12
        ____SPKR_DUTY____4      ; 4
ad4:    ldx     $A8FF           ; 8
        ____SPKR_DUTY____4      ; 12
vs4:    lda     $99FF           ; 16
        and     #HAS_BYTE       ; 18
        beq     no_vid4         ; 20/21
vd4:    lda     $98FF           ; 24
        stx     next+1          ; 27
        WASTE_12                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid4:
        WASTE_3                 ; 24
        stx   next+1            ; 27
vs4b:   lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid4b        ; 35/36
vd4b:   lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid4b:
ad4b:   ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

video_data_patches:
                .word vd0
                .word vd0b
                .word vd1
                .word vd1b
                .word vd2
                .word vd2b
                .word vd3
                .word vd3b
                .word vd4
                .word vd4b
                .word vd5
                .word vd5b
                .word vd6
                .word vd6b
                .word vd7
                .word vd7b
                .word vd8
                .word vd8b
                .word vd9
                .word vd9b
                .word vd10
                .word vd10b
                .word vd11
                .word vd11b
                .word vd12
                .word vd12b
                .word vd13
                .word vd13b
                .word vd14
                .word vd14b
                .word vd15
                .word vd15b
                .word vd16
                .word vd16b
                .word vd17
                .word vd17b
                .word vd18
                .word vd18b
                .word vd19
                .word vd19b
                .word vd20
                .word vd20b
                .word vd21
                .word vd21b
                .word vd22
                .word vd22b
                .word vd23
                .word vd23b
                .word vd24
                .word vd24b
                .word vd25
                .word vd25b
                .word vd26
                .word vd26b
                .word vd27
                .word vd27b
                .word vd28
                .word vd28b
                .word vd29
                .word vd29b
                .word vd30
                .word vd30b
                .word vd31
                .word vd31b
                .word $0000

.align 256
.assert * = _SAMPLES_BASE + $500, error
duty_cycle5:                    ; end spkr at 13
        ____SPKR_DUTY____4      ; 4
ad5:    ldx     $A8FF           ; 8
        ____SPKR_DUTY____5      ; 13
vs5:    lda     $99FF           ; 17
        and     #HAS_BYTE       ; 19
        beq     no_vid5         ; 21/22
vd5:    lda     $98FF           ; 25
        stx     next+1          ; 28
        WASTE_11                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid5:
        WASTE_2                 ; 24
        stx   next+1            ; 27
vs5b:   lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid5b        ; 35/36
vd5b:   lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid5b:
ad5b:   ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

audio_status_patches:
                .word as31
                .word $0000

audio_data_patches:
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
                .word adv
                .word ad31
                .word ad31b
                .word $0000

.align 256
.assert * = _SAMPLES_BASE + $600, error
duty_cycle6:                    ; end spkr at 14
        ____SPKR_DUTY____4      ; 4
ad6:    ldx     $A8FF           ; 8
        WASTE_2                 ; 10
        ____SPKR_DUTY____4      ; 14
vs6:    lda     $99FF           ; 18
        and     #HAS_BYTE       ; 20
        beq     no_vid6         ; 22/23
vd6:    lda     $98FF           ; 26
        stx     next+1          ; 29
        WASTE_10                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid6:
        ABS_STX next+1          ; 27 stx absolute
vs6b:   lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid6b        ; 35/36
vd6b:   lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid6b:
ad6b:   ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $700, error
duty_cycle7:                    ; end spkr at 15
        ____SPKR_DUTY____4      ; 4
ad7:    ldx     $A8FF           ; 8
        WASTE_3                 ; 11
        ____SPKR_DUTY____4      ; 15
vs7:    lda     $99FF           ; 19
        and     #HAS_BYTE       ; 21
        beq     no_vid7         ; 23/24
vd7:    lda     $98FF           ; 27
        stx     next+1          ; 30
        WASTE_9                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid7:
        stx     next+1          ; 27
vs7b:   lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid7b        ; 35/36
vd7b:   lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid7b:
ad7b:   ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $800, error
duty_cycle8:                    ; end spkr at 16
        ____SPKR_DUTY____4      ; 4
ad8:    ldx     $A8FF           ; 8
vs8:    lda     $99FF           ; 12
        ____SPKR_DUTY____4      ; 16
        and     #HAS_BYTE       ; 18
        beq     no_vid8         ; 20/21
vd8:    lda     $98FF           ; 24
        stx     next+1          ; 27
        WASTE_12                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid8:
        WASTE_3                 ; 24
        stx     next+1          ; 27
vs8b:   lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid8b        ; 35/36
vd8b:   lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid8b:
ad8b:   ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83


.align 256
.assert * = _SAMPLES_BASE + $900, error
duty_cycle9:                    ; end spkr at 17
        ____SPKR_DUTY____4      ; 4
ad9:    ldx     $A8FF           ; 8
vs9:    lda     $99FF           ; 12
        ____SPKR_DUTY____5      ; 17
        and     #HAS_BYTE       ; 19
        beq     no_vid9         ; 21/22
vd9:    lda     $98FF           ; 25
        stx     next+1          ; 28
        WASTE_11                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid9:
        WASTE_2                 ; 24
        stx     next+1          ; 27
vs9b:   lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid9b        ; 35/36
vd9b:   lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid9b:
ad9b:   ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $A00, error
duty_cycle10:                    ; end spkr at 18
        ____SPKR_DUTY____4      ; 4
ad10:   ldx     $A8FF           ; 8
vs10:   lda     $99FF           ; 12
        WASTE_2                 ; 14
        ____SPKR_DUTY____4      ; 18
        and     #HAS_BYTE       ; 20
        beq     no_vid10        ; 22/23
vd10:   lda     $98FF           ; 26
        stx     next+1          ; 29
        WASTE_10                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid10:
        ABS_STX next+1          ; 27 stx absolute
vs10b:  lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid10b       ; 35/36
vd10b:  lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid10b:
ad10b:  ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83


.align 256
.assert * = _SAMPLES_BASE + $B00, error
duty_cycle11:                    ; end spkr at 19
        ____SPKR_DUTY____4      ; 4
ad11:   ldx     $A8FF           ; 8
vs11:   lda     $99FF           ; 12
        WASTE_3                 ; 15
        ____SPKR_DUTY____4      ; 19
        and     #HAS_BYTE       ; 21
        beq     no_vid11        ; 23/24
vd11:   lda     $98FF           ; 27
        stx     next+1          ; 30
        WASTE_9                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid11:
        stx     next+1          ; 27
vs11b:  lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid11b       ; 35/36
vd11b:  lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid11b:
ad11b:  ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83


.align 256
.assert * = _SAMPLES_BASE + $C00, error
duty_cycle12:                    ; end spkr at 20
        ____SPKR_DUTY____4      ; 4
ad12:   ldx     $A8FF           ; 8
vs12:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        WASTE_2                 ; 16
        ____SPKR_DUTY____4      ; 20
        beq     no_vid12        ; 22/23
vd12:   lda     $98FF           ; 26
        stx     next+1          ; 29
        WASTE_10                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid12:
        ABS_STX next+1          ; 27 stx absolute
vs12b:  lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid12b       ; 35/36
vd12b:  lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid12b:
ad12b:  ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $D00, error
duty_cycle13:                    ; end spkr at 21
        ____SPKR_DUTY____4      ; 4
ad13:   ldx     $A8FF           ; 8
vs13:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid13        ; 16/17
        ____SPKR_DUTY____5      ; 21
vd13:   lda     $98FF           ; 25
        stx     next+1          ; 28
        WASTE_11                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid13:
        ____SPKR_DUTY____4      ; 21
        stx     next+1          ; 24
        WASTE_3                 ; 27
vs13b:  lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid13b       ; 35/36
vd13b:  lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid13b:
ad13b:  ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $E00, error
duty_cycle14:                    ; end spkr at 22
        ____SPKR_DUTY____4      ; 4
ad14:   ldx     $A8FF           ; 8
vs14:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid14        ; 16/17
        WASTE_2                 ; 18
        ____SPKR_DUTY____4      ; 22
vd14:   lda     $98FF           ; 26
        stx     next+1          ; 29
        WASTE_10                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid14:
        ____SPKR_DUTY____5      ; 22
        stx     next+1          ; 25
        WASTE_2                 ; 27
vs14b:  lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid14b       ; 35/36
vd14b:  lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid14b:
ad14b:  ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $F00, error
duty_cycle15:                    ; end spkr at 23
        ____SPKR_DUTY____4      ; 4
ad15:   ldx     $A8FF           ; 8
vs15:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid15        ; 16/17
        WASTE_3                 ; 19
        ____SPKR_DUTY____4      ; 23
vd15:   lda     $98FF           ; 27
        stx     next+1          ; 30
        WASTE_9                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid15:
        WASTE_2                 ; 19
        ____SPKR_DUTY____4      ; 23
        ABS_STX next+1          ; 27 stx absolute
vs15b:  lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid15b       ; 35/36
vd15b:  lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid15b:
ad15b:  ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1000, error
duty_cycle16:                    ; end spkr at 24
        ____SPKR_DUTY____4      ; 4
ad16:   ldx     $A8FF           ; 8
vs16:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid16        ; 16/17
vd16:   lda     $98FF           ; 20
        ____SPKR_DUTY____4      ; 24
        stx     next+1          ; 27
        WASTE_12                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid16:
        stx     next+1          ; 20
        ____SPKR_DUTY____4      ; 24
        WASTE_3                 ; 27
vs16b:  lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid16b       ; 35/36
vd16b:  lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid16b:
ad16b:  ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1100, error
duty_cycle17:                    ; end spkr at 25
        ____SPKR_DUTY____4      ; 4
ad17:   ldx     $A8FF           ; 8
vs17:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid17        ; 16/17
vd17:   lda     $98FF           ; 20
        ____SPKR_DUTY____5      ; 25
        stx     next+1          ; 28
        WASTE_11                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid17:
        stx     next+1          ; 20
        ____SPKR_DUTY____5      ; 25
        WASTE_2                 ; 27
vs17b:  lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid17b       ; 35/36
vd17b:  lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid17b:
ad17b:  ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1200, error
duty_cycle18:                    ; end spkr at 26
        ____SPKR_DUTY____4      ; 4
ad18:   ldx     $A8FF           ; 8
vs18:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid18        ; 16/17
vd18:   lda     $98FF           ; 20
        WASTE_2                 ; 22
        ____SPKR_DUTY____4      ; 26
        stx     next+1          ; 29
        WASTE_10                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid18:
        ABS_STX next+1          ; 21
        ____SPKR_DUTY____5      ; 26
vs18b:  lda     $99FF           ; 30
        and     has_byte_zp     ; 33
        beq     no_vid18b       ; 35/36
vd18b:  lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid18b:
ad18b:  ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1300, error
duty_cycle19:                    ; end spkr at 27
        ____SPKR_DUTY____4      ; 4
ad19:   ldx     $A8FF           ; 8
vs19:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid19        ; 16/17
vd19:   lda     $98FF           ; 20
        WASTE_2                 ; 22
        ____SPKR_DUTY____5      ; 27
        stx     next+1          ; 30
        WASTE_9                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid19:
        WASTE_2                 ; 19
        ABS_STX next+1          ; 23 stx absolute
        ____SPKR_DUTY____4      ; 27
vs19b:  lda     $99FF           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid19b       ; 35/36
vd19b:  lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid19b:
ad19b:  ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1400, error
duty_cycle20:                    ; end spkr at 28
        ____SPKR_DUTY____4      ; 4
ad20:   ldx     $A8FF           ; 8
vs20:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid20        ; 16/17
vd20:   lda     $98FF           ; 20
        WASTE_4                 ; 24
        ____SPKR_DUTY____4      ; 28
        stx     next+1          ; 31
        WASTE_8                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid20:
        stx     next+1          ; 20
vs20b:  lda     $99FF           ; 24
        ____SPKR_DUTY____4      ; 28
        WASTE_3                 ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid20b       ; 35/36
vd20b:  lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid20b:
ad20b:  ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1500, error
duty_cycle21:                    ; end spkr at 29
        ____SPKR_DUTY____4      ; 4
ad21:   ldx     $A8FF           ; 8
vs21:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid21        ; 16/17
vd21:   lda     $98FF           ; 20
        stx     next+1          ; 23
        WASTE_2                 ; 25
        ____SPKR_DUTY____4      ; 29
        WASTE_10                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid21:
        ABS_STX next+1          ; 21 stx absolute
vs21b:  lda     $99FF           ; 25
        ____SPKR_DUTY____4      ; 29
        WASTE_2                 ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid21b       ; 35/36
vd21b:  lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid21b:
ad21b:  ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1600, error
duty_cycle22:                    ; end spkr at 30
        ____SPKR_DUTY____4      ; 4
ad22:   ldx     $A8FF           ; 8
vs22:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid22        ; 16/17
vd22:   lda     $98FF           ; 20
        stx     next+1          ; 23
        WASTE_3                 ; 26
        ____SPKR_DUTY____4      ; 30
        WASTE_9                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid22:
        stx     next+1          ; 20
vs22b:  lda     $99FF           ; 24
        and     #HAS_BYTE       ; 26
        ____SPKR_DUTY____4      ; 30
        beq     no_vid22b       ; 32/33
        WASTE_3                 ; 35
vd22b:  lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid22b:
ad22b:  ldx     $A8FF           ; 37
        stx     next+1          ; 40
        KBD_LOAD_13             ; 53
        WASTE_24                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1700, error
duty_cycle23:                    ; end spkr at 31
        ____SPKR_DUTY____4      ; 4
ad23:   ldx     $A8FF           ; 8
vs23:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid23        ; 16/17
vd23:   lda     $98FF           ; 20
        stx     next+1          ; 23
        WASTE_4                 ; 27
        ____SPKR_DUTY____4      ; 31
        WASTE_8                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid23:
        stx     next+1          ; 20
vs23b:  lda     $99FF           ; 24
        and     #HAS_BYTE       ; 26
        ____SPKR_DUTY____5      ; 31
        WASTE_2                 ; 33
        beq     no_vid23b       ; 35/36
vd23b:  lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid23b:
ad23b:  ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83


.align 256
.assert * = _SAMPLES_BASE + $1800, error
duty_cycle24:                    ; end spkr at 32
        ____SPKR_DUTY____4      ; 4
ad24:   ldx     $A8FF           ; 8
vs24:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid24        ; 16/17
vd24:   lda     $98FF           ; 20
        stx     next+1          ; 23
        WASTE_5                 ; 28
        ____SPKR_DUTY____4      ; 32
        WASTE_7                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid24:
        WASTE_2                 ; 19
vs24b:  lda     $99FF           ; 23
        and     #HAS_BYTE       ; 25
        beq     no_vid24b       ; 27/28
        ____SPKR_DUTY____5      ; 32
        stx     next+1          ; 35
vd24b:  lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid24b:
        ____SPKR_DUTY____4      ; 32
ad24b:  ldx     $A8FF           ; 36
        stx     next+1          ; 39
        KBD_LOAD_13             ; 52
        WASTE_25                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1900, error
duty_cycle25:                    ; end spkr at 33
        ____SPKR_DUTY____4      ; 4
ad25:   ldx     $A8FF           ; 8
vs25:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid25        ; 16/17
vd25:   lda     $98FF           ; 20
        stx     next+1          ; 23
        WASTE_6                 ; 29
        ____SPKR_DUTY____4      ; 33
        WASTE_6                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid25:
        stx     next+1          ; 20
vs25b:  lda     $99FF           ; 24
        and     #HAS_BYTE       ; 26
        beq     no_vid25b       ; 28/29
        ____SPKR_DUTY____5      ; 33
        WASTE_2                 ; 35
vd25b:  lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid25b:
        ____SPKR_DUTY____4      ; 33
ad25b:  ldx     $A8FF           ; 37
        stx     next+1          ; 40
        KBD_LOAD_13             ; 53
        WASTE_24                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1A00, error
duty_cycle26:                    ; end spkr at 34
        ____SPKR_DUTY____4      ; 4
ad26:   ldx     $A8FF           ; 8
vs26:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid26        ; 16/17
vd26:   lda     $98FF           ; 20
        stx     next+1          ; 23
        WASTE_7                 ; 30
        ____SPKR_DUTY____4      ; 34
        WASTE_5                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid26:
vs26b:  lda     $99FF           ; 21
        and     #HAS_BYTE       ; 23
        beq     no_vid26b       ; 25/26
vd26b:  lda     $98FF           ; 29
        ____SPKR_DUTY____5      ; 34
        WASTE_2                 ; 36
        stx     next+1          ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid26b:
        WASTE_4                 ; 30
        ____SPKR_DUTY____4      ; 34
ad26b:  ldx     $A8FF           ; 38
        stx     next+1          ; 41
        KBD_LOAD_13             ; 54
        WASTE_23                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1B00, error
duty_cycle27:                    ; end spkr at 35
        ____SPKR_DUTY____4      ; 4
ad27:   ldx     $A8FF           ; 8
vs27:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid27        ; 16/17
vd27:   lda     $98FF           ; 20
        stx     next+1          ; 23
        WASTE_8                 ; 31
        ____SPKR_DUTY____4      ; 35
        WASTE_4                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid27:
        stx     next+1          ; 20
        WASTE_2                 ; 22
vs27b:  lda     $99FF           ; 26
        and     #HAS_BYTE       ; 28
        beq     no_vid27b       ; 30/31
        ____SPKR_DUTY____5      ; 35
vd27b:  lda     $98FF           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid27b:
        ____SPKR_DUTY____4      ; 35
ad27b:  ldx     $A8FF           ; 39
        stx     next+1          ; 42
        KBD_LOAD_13             ; 55
        WASTE_22                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1C00, error
duty_cycle28:                    ; end spkr at 36
        ____SPKR_DUTY____4      ; 4
ad28:   ldx     $A8FF           ; 8
vs28:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid28        ; 16/17
vd28:   lda     $98FF           ; 20
        stx     next+1          ; 23
        WASTE_9                 ; 32
        ____SPKR_DUTY____4      ; 36
        WASTE_3                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid28:
        WASTE_2                 ; 19
vs28b:  lda     $99FF           ; 23
        and     #HAS_BYTE       ; 25
        beq     no_vid28b       ; 27/28
vd28b:  lda     $98FF           ; 31
        ____SPKR_DUTY____5      ; 36
        stx     next+1          ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid28b:
        WASTE_4                 ; 32
        ____SPKR_DUTY____4      ; 36
ad28b:  ldx     $A8FF           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83


.align 256
.assert * = _SAMPLES_BASE + $1D00, error
duty_cycle29:                    ; end spkr at 37
        ____SPKR_DUTY____4      ; 4
ad29:   ldx     $A8FF           ; 8
vs29:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid29        ; 16/17
vd29:   lda     $98FF           ; 20
        stx     next+1          ; 23
        WASTE_10                ; 33
        ____SPKR_DUTY____4      ; 37
        WASTE_2                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid29:
        stx     next+1          ; 20
vs29b:  lda     $99FF           ; 24
        and     #HAS_BYTE       ; 26
        beq     no_vid29b       ; 28/29
vd29b:  lda     $98FF           ; 32
        ____SPKR_DUTY____5      ; 37
        WASTE_2                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid29b:
        WASTE_4                 ; 33
        ____SPKR_DUTY____4      ; 37
ad29b:  ldx     $A8FF           ; 41
        stx     next+1          ; 44
        KBD_LOAD_13             ; 57
        WASTE_20                ; 77
        jmp     (next)          ; 83


.align 256
.assert * = _SAMPLES_BASE + $1E00, error
; Duty cycle 30 must toggle off speaker at cycle 38, but we would have to jump
; to video_direct at cycle 39, so this one uses different entry points to
; the video handler to fix this.
duty_cycle30:                    ; end spkr at 38
        ____SPKR_DUTY____4      ; 4
ad30:   ldx     $A8FF           ; 8
vs30:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid30        ; 16/17
vd30:   lda     $98FF           ; 20
        WASTE_11                ; 31
        jmp     video_tog_spkr  ; 34=>83 (turns spkr off, jumps to next)

no_vid30:
        jmp     duty_cycle30_v2 ; 20=>83 (takes 63 cycles)

no_vid30b:
        WASTE_2                 ; 34
        ____SPKR_DUTY____4      ; 38
ad30b:  ldx     $A8FF           ; 42
        stx     next+1          ; 45
        KBD_LOAD_13             ; 58
        WASTE_19                ; 77
        jmp     (next)          ; 83

duty_cycle30_v2:                ; Alternate entry point for duty cycle 30
vs30b:  lda     $99FF           ; 24
        and     has_byte_zp     ; 27
        bne     vd30b           ; 29/30
        jmp     no_vid30b       ; 32

vd30b:  lda     $98FF           ; 34

; -----------------------------------------------------------------
; VIDEO HANDLER

video_tog_spkr:                 ; Alternate entry point for duty cycle 30
        ____SPKR_DUTY____4      ; 38
        ABS_STX next+1          ; 42

; Video handler expects the video byte in A register.
; Video handler must take 41 cycles on every code path.
video_direct:
        bmi     control           ; 2/3   Is it a control byte?
set_pixel:                        ;       No, it is a data byte
        ldy     last_base         ; 5     Finish storing base (we didn't have enough cycles to do all of it in set_base).
        tax                       ; 7     Save data byte to X
page_ptr_a:
        lda     (page1_addr_ptr),y; 12    Load base pointer high byte from base array
        sta     store_dest+1      ; 15    Store it to destination pointer high byte
        txa                       ; 17    Restore data byte
        ldy     last_offset       ; 20    Load the offset to the start of the base
        sta     (store_dest),y    ; 26    Store data byte
        iny                       ; 28    Increment offset
        sty     last_offset       ; 31    and store it.
        ABS_STZ got_offset        ; 35    Reset the offset-received flag. (stz ABSOLUTE to waste one cycle)
        jmp     (next)            ; 41    Done, go to next duty cycle

control:                          ;       It is a control byte
        cmp     #$FF              ; 5     Is it the page toggle command?
        beq     toggle_page       ; 7/8   Yes
        and     #%01111111        ; 9     No. Strip high byte to get the positive value

        ldx     got_offset        ; 12    Did we get an offset byte earlier?
        bne     set_base          ; 14/15 Yes, so this one is a base byte

set_offset:                       ;       No, so set offset
        sta     last_offset       ; 17    Store offset
        inc     got_offset        ; 22    Set the offset-received flag
adv:    ldx     $A8FF             ; 26    We have extra time, so load audio byte
        stx     next+1            ; 29    And update jump destination accordingly
        WASTE_6                   ; 35    (So much extra cycles!)
        jmp     (next)            ; 41    Done, go to next duty cycle

set_base:                         ;       This is a base byte
        asl     a                 ; 17    Double it as the base array is uint16
        tay                       ; 19    Transfer to Y for array access
page_ptr_b:
        lda     (page1_addr_ptr),y; 24    Load base pointer low byte from base array
        sta     store_dest        ; 27    Store it to destination pointer low byte
        iny                       ; 29    Increment Y for high byte
        sty     last_base         ; 32    Store it, we don't have time here, we'll finish updating base on a data byte
        stz     got_offset        ; 35    Reset the offset-received flag
        jmp     (next)            ; 41    Done, go to next duty cycle

toggle_page:                      ;       Page toggling command
        ldx     page              ; 11    Get next page to activate
        sta     $C054,x           ; 16    Toggle soft switch
        lda     page_addr_ptr,x   ; 20    Load the page's base address high byte
        sta     page_ptr_a+1      ; 24    Update the base array pointer (low byte handler in set_base)
        sta     page_ptr_b+1      ; 28    Update the base array pointer (high byte handler in set_pixel)
        txa                       ; 30    Transfer X to A to invert it
        eor     #$01              ; 32    Invert (toggle page for next time)
        sta     page              ; 35    Store next page
        jmp     (next)            ; 41    Done, go to next duty cycle
; -----------------------------------------------------------------

.align 256
.assert * = _SAMPLES_BASE + $1F00, error
duty_cycle31:                    ; end spkr at 39
        ____SPKR_DUTY____4      ; 4
ad31:   ldx     $A8FF           ; 8
vs31:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid31        ; 16/17
vd31:   lda     $98FF           ; 20
        stx     next+1          ; 23
        WASTE_12                ; 35
        ____SPKR_DUTY____4      ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid31:
        stx     next+1          ; 20
vs31b:  lda     $99FF           ; 24
        and     #HAS_BYTE       ; 26
        beq     no_vid31b       ; 28/29
vd31b:  lda     $98FF           ; 32
        WASTE_2                 ; 34
        ____SPKR_DUTY____5      ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid31b:
        WASTE_6                 ; 35
        ____SPKR_DUTY____4      ; 39
ad31b:  ldx     $A8FF           ; 43
        stx     next+1          ; 46
        KBD_LOAD_13             ; 59
        WASTE_18                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE+$2000, error
break_out:
        lda     #$01            ; Reenable IRQ and flush
        jsr     _simple_serial_set_irq
        jsr     _simple_serial_flush
        lda     #$2F            ; SURL_CLIENT_READY
        jmp     _serial_putc_direct

.align 256
page1_addrs_arr:.res (N_BASES*2)          ; Base addresses arrays
.align 256                                ; Aligned for correct cycle counting
page2_addrs_arr:.res (N_BASES*2)

page_addr_ptr:  .byte <(page2_addr_ptr)   ; Base addresses pointer for page 2
                .byte <(page1_addr_ptr)   ; Base addresses pointer for page 1

        .bss
stop:           .res 1
dummy_abs:      .res 2
prevspd:        .res 1
next_slow:      .res 2
