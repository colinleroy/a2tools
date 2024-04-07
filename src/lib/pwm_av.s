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

.macro ABS_STX  zpvar
        .byte   $8E             ; stx absolute
        .byte   zpvar
        .byte   $00
.endmacro

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
        jmp     _serial_putc_direct
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

.align 256
_SAMPLES_BASE = *
.assert * = $6000, error
duty_cycle0:                    ; end spkr at 8
        ____SPKR_DUTY____4      ; 4
        ____SPKR_DUTY____4      ; 8
vs0:    lda     $C099           ; 12
ad0:    ldx     $C0A8           ; 16
        and     #HAS_BYTE       ; 18
        beq     no_vid0         ; 20/21
vd0:    lda     $C098           ; 24
        stx     next+1          ; 27
        WASTE_12                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid0:
        stx     next+1          ; 24
        WASTE_3                 ; 27
        lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid0b        ; 35/36
vd0b:   lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid0b:
        WASTE_3                 ; 39
ad0b:   ldx     $C0A8           ; 43
        stx     next+1          ; 46
        KBD_LOAD_13             ; 59
        WASTE_18                ; 77
        jmp     (next)          ; 83

; -----------------------------------------------------------------
_pwm:
        pha
        ; Disable interrupts
        lda     #$00
        jsr     _simple_serial_set_irq

        pla
        ; Setup pointers
        jsr     setup_pointers

        ; Start with silence
as31:   lda     $C0A9
        and     #HAS_BYTE
        beq     as31
        jmp     duty_cycle31
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
        lda     serial_status_reg+1
        sta     audio_status
        lda     serial_status_reg+2
        sta     audio_status+1

        lda     serial_data_reg+1
        sta     audio_data
        lda     serial_data_reg+2
        sta     audio_data+1

        lda     $C0A8+2
        sta     $C098+2
        lda     $C0A8+3
        sta     $C098+3

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
ad1:    ldx     $C0A8           ; 13
vs1:    lda     $C099           ; 17
        and     #HAS_BYTE       ; 19
        beq     no_vid1         ; 21/22
vd1:    lda     $C098           ; 25
        stx     next+1          ; 28
        WASTE_11                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid1:
        WASTE_2                 ; 24
        stx     next+1          ; 27
        lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid1b        ; 35/36
vd1b:   lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid1b:
ad1b:   ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $200, error
duty_cycle2:                    ; end spkr at 10
        ____SPKR_DUTY____4      ; 4
        WASTE_2                 ; 6
        ____SPKR_DUTY____4      ; 10
ad2:    ldx     $C0A8           ; 14
vs2:    lda     $C099           ; 18
        and     #HAS_BYTE       ; 20
        beq     no_vid2         ; 22/23
vd2:    lda     $C098           ; 26
        stx     next+1          ; 29
        WASTE_10                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid2:
        ABS_STX next+1          ; 27 stx absolute
        lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid2b        ; 35/36
vd2b:   lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid2b:
ad2b:   ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $300, error
duty_cycle3:                    ; end spkr at 11
        ____SPKR_DUTY____4      ; 4
        WASTE_3                 ; 7
        ____SPKR_DUTY____4      ; 11
ad3:    ldx     $C0A8           ; 15
vs3:    lda     $C099           ; 19
        and     #HAS_BYTE       ; 21
        beq     no_vid3         ; 23/24
vd3:    lda     $C098           ; 27
        stx     next+1          ; 30
        WASTE_9                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid3:
        stx   next+1            ; 27
        lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid3b        ; 35/36
vd3b:   lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid3b:
ad3b:   ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $400, error
duty_cycle4:                    ; end spkr at 12
        ____SPKR_DUTY____4      ; 4
ad4:    ldx     $C0A8           ; 8
        ____SPKR_DUTY____4      ; 12
vs4:    lda     $C099           ; 16
        and     #HAS_BYTE       ; 18
        beq     no_vid4         ; 20/21
vd4:    lda     $C098           ; 24
        stx     next+1          ; 27
        WASTE_12                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid4:
        WASTE_3                 ; 24
        stx   next+1            ; 27
        lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid4b        ; 35/36
vd4b:   lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid4b:
ad4b:   ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $500, error
duty_cycle5:                    ; end spkr at 13
        ____SPKR_DUTY____4      ; 4
ad5:    ldx     $C0A8           ; 8
        ____SPKR_DUTY____5      ; 13
vs5:    lda     $C099           ; 17
        and     #HAS_BYTE       ; 19
        beq     no_vid5         ; 21/22
vd5:    lda     $C098           ; 25
        stx     next+1          ; 28
        WASTE_11                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid5:
        WASTE_2                 ; 24
        stx   next+1            ; 27
        lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid5b        ; 35/36
vd5b:   lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid5b:
ad5b:   ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $600, error
duty_cycle6:                    ; end spkr at 14
        ____SPKR_DUTY____4      ; 4
ad6:    ldx     $C0A8           ; 8
        WASTE_2                 ; 10
        ____SPKR_DUTY____4      ; 14
vs6:    lda     $C099           ; 18
        and     #HAS_BYTE       ; 20
        beq     no_vid6         ; 22/23
vd6:    lda     $C098           ; 26
        stx     next+1          ; 29
        WASTE_10                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid6:
        ABS_STX next+1          ; 27 stx absolute
        lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid6b        ; 35/36
vd6b:   lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid6b:
ad6b:   ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $700, error
duty_cycle7:                    ; end spkr at 15
        ____SPKR_DUTY____4      ; 4
ad7:    ldx     $C0A8           ; 8
        WASTE_3                 ; 11
        ____SPKR_DUTY____4      ; 15
vs7:    lda     $C099           ; 19
        and     #HAS_BYTE       ; 21
        beq     no_vid7         ; 23/24
vd7:    lda     $C098           ; 27
        stx     next+1          ; 30
        WASTE_9                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid7:
        stx     next+1          ; 27
        lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid7b        ; 35/36
vd7b:   lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid7b:
ad7b:   ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $800, error
duty_cycle8:                    ; end spkr at 16
        ____SPKR_DUTY____4      ; 4
ad8:    ldx     $C0A8           ; 8
vs8:    lda     $C099           ; 12
        ____SPKR_DUTY____4      ; 16
        and     #HAS_BYTE       ; 18
        beq     no_vid8         ; 20/21
vd8:    lda     $C098           ; 24
        stx     next+1          ; 27
        WASTE_12                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid8:
        WASTE_3                 ; 24
        stx     next+1          ; 27
        lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid8b        ; 35/36
vd8b:   lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid8b:
ad8b:   ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83


.align 256
.assert * = _SAMPLES_BASE + $900, error
duty_cycle9:                    ; end spkr at 17
        ____SPKR_DUTY____4      ; 4
ad9:    ldx     $C0A8           ; 8
vs9:    lda     $C099           ; 12
        ____SPKR_DUTY____5      ; 17
        and     #HAS_BYTE       ; 19
        beq     no_vid9         ; 21/22
vd9:    lda     $C098           ; 25
        stx     next+1          ; 28
        WASTE_11                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid9:
        WASTE_2                 ; 24
        stx     next+1          ; 27
        lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid9b        ; 35/36
vd9b:   lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid9b:
ad9b:   ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $A00, error
duty_cycle10:                    ; end spkr at 18
        ____SPKR_DUTY____4      ; 4
ad10:   ldx     $C0A8           ; 8
vs10:   lda     $C099           ; 12
        WASTE_2                 ; 14
        ____SPKR_DUTY____4      ; 18
        and     #HAS_BYTE       ; 20
        beq     no_vid10        ; 22/23
vd10:   lda     $C098           ; 26
        stx     next+1          ; 29
        WASTE_10                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid10:
        ABS_STX next+1          ; 27 stx absolute
        lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid10b       ; 35/36
vd10b:  lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid10b:
ad10b:  ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83


.align 256
.assert * = _SAMPLES_BASE + $B00, error
duty_cycle11:                    ; end spkr at 19
        ____SPKR_DUTY____4      ; 4
ad11:   ldx     $C0A8           ; 8
vs11:   lda     $C099           ; 12
        WASTE_3                 ; 15
        ____SPKR_DUTY____4      ; 19
        and     #HAS_BYTE       ; 21
        beq     no_vid11        ; 23/24
vd11:   lda     $C098           ; 27
        stx     next+1          ; 30
        WASTE_9                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid11:
        stx     next+1          ; 27
        lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid11b       ; 35/36
vd11b:  lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid11b:
ad11b:  ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83


.align 256
.assert * = _SAMPLES_BASE + $C00, error
duty_cycle12:                    ; end spkr at 20
        ____SPKR_DUTY____4      ; 4
ad12:   ldx     $C0A8           ; 8
vs12:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        WASTE_2                 ; 16
        ____SPKR_DUTY____4      ; 20
        beq     no_vid12        ; 22/23
vd12:   lda     $C098           ; 26
        stx     next+1          ; 29
        WASTE_10                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid12:
        ABS_STX next+1          ; 27 stx absolute
        lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid12b       ; 35/36
vd12b:  lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid12b:
ad12b:  ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $D00, error
duty_cycle13:                    ; end spkr at 21
        ____SPKR_DUTY____4      ; 4
ad13:   ldx     $C0A8           ; 8
vs13:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid13        ; 16/17
        ____SPKR_DUTY____5      ; 21
vd13:   lda     $C098           ; 25
        stx     next+1          ; 28
        WASTE_11                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid13:
        ____SPKR_DUTY____4      ; 21
        stx     next+1          ; 24
        WASTE_3                 ; 27
        lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid13b       ; 35/36
vd13b:  lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid13b:
ad13b:  ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $E00, error
duty_cycle14:                    ; end spkr at 22
        ____SPKR_DUTY____4      ; 4
ad14:   ldx     $C0A8           ; 8
vs14:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid14        ; 16/17
        WASTE_2                 ; 18
        ____SPKR_DUTY____4      ; 22
vd14:   lda     $C098           ; 26
        stx     next+1          ; 29
        WASTE_10                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid14:
        ____SPKR_DUTY____5      ; 22
        stx     next+1          ; 25
        WASTE_2                 ; 27
        lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid14b       ; 35/36
vd14b:  lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid14b:
ad14b:  ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $F00, error
duty_cycle15:                    ; end spkr at 23
        ____SPKR_DUTY____4      ; 4
ad15:   ldx     $C0A8           ; 8
vs15:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid15        ; 16/17
        WASTE_3                 ; 19
        ____SPKR_DUTY____4      ; 23
vd15:   lda     $C098           ; 27
        stx     next+1          ; 30
        WASTE_9                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid15:
        WASTE_2                 ; 19
        ____SPKR_DUTY____4      ; 23
        ABS_STX next+1          ; 27 stx absolute
        lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid15b       ; 35/36
vd15b:  lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid15b:
ad15b:  ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1000, error
duty_cycle16:                    ; end spkr at 24
        ____SPKR_DUTY____4      ; 4
ad16:   ldx     $C0A8           ; 8
vs16:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid16        ; 16/17
vd16:   lda     $C098           ; 20
        ____SPKR_DUTY____4      ; 24
        stx     next+1          ; 27
        WASTE_12                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid16:
        stx     next+1          ; 20
        ____SPKR_DUTY____4      ; 24
        WASTE_3                 ; 27
vs16b:  lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid16b       ; 35/36
vd16b:  lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid16b:
ad16b:  ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1100, error
duty_cycle17:                    ; end spkr at 25
        ____SPKR_DUTY____4      ; 4
ad17:   ldx     $C0A8           ; 8
vs17:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid17        ; 16/17
vd17:   lda     $C098           ; 20
        ____SPKR_DUTY____5      ; 25
        stx     next+1          ; 28
        WASTE_11                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid17:
        stx     next+1          ; 20
        ____SPKR_DUTY____5      ; 25
        WASTE_2                 ; 27
vs17b:  lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid17b       ; 35/36
vd17b:  lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid17b:
ad17b:  ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1200, error
duty_cycle18:                    ; end spkr at 26
        ____SPKR_DUTY____4      ; 4
ad18:   ldx     $C0A8           ; 8
vs18:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid18        ; 16/17
vd18:   lda     $C098           ; 20
        WASTE_2                 ; 22
        ____SPKR_DUTY____4      ; 26
        stx     next+1          ; 29
        WASTE_10                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid18:
vs18b:  ABS_STX next+1          ; 21
        ____SPKR_DUTY____5      ; 26
        lda     $C099           ; 30
        and     has_byte_zp     ; 33
        beq     no_vid18b       ; 35/36
vd18b:  lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid18b:
ad18b:  ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1300, error
duty_cycle19:                    ; end spkr at 27
        ____SPKR_DUTY____4      ; 4
ad19:   ldx     $C0A8           ; 8
vs19:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid19        ; 16/17
vd19:   lda     $C098           ; 20
        WASTE_2                 ; 22
        ____SPKR_DUTY____5      ; 27
        stx     next+1          ; 30
        WASTE_9                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid19:
        WASTE_2                 ; 19
        ABS_STX next+1          ; 23 stx absolute
        ____SPKR_DUTY____4      ; 27
vs19b:  lda     $C099           ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid19b       ; 35/36
vd19b:  lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid19b:
ad19b:  ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1400, error
duty_cycle20:                    ; end spkr at 28
        ____SPKR_DUTY____4      ; 4
ad20:   ldx     $C0A8           ; 8
vs20:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid20        ; 16/17
vd20:   lda     $C098           ; 20
        WASTE_4                 ; 24
        ____SPKR_DUTY____4      ; 28
        stx     next+1          ; 31
        WASTE_8                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid20:
        stx     next+1          ; 20
vs20b:  lda     $C099           ; 24
        ____SPKR_DUTY____4      ; 28
        WASTE_3                 ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid20b       ; 35/36
vd20b:  lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid20b:
ad20b:  ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1500, error
duty_cycle21:                    ; end spkr at 29
        ____SPKR_DUTY____4      ; 4
ad21:   ldx     $C0A8           ; 8
vs21:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid21        ; 16/17
vd21:   lda     $C098           ; 20
        stx     next+1          ; 23
        WASTE_2                 ; 25
        ____SPKR_DUTY____4      ; 29
        WASTE_10                ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid21:
        ABS_STX next+1          ; 21 stx absolute
vs21b:  lda     $C099           ; 25
        ____SPKR_DUTY____4      ; 29
        WASTE_2                 ; 31
        and     #HAS_BYTE       ; 33
        beq     no_vid21b       ; 35/36
vd21b:  lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid21b:
ad21b:  ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1600, error
duty_cycle22:                    ; end spkr at 30
        ____SPKR_DUTY____4      ; 4
ad22:   ldx     $C0A8           ; 8
vs22:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid22        ; 16/17
vd22:   lda     $C098           ; 20
        stx     next+1          ; 23
        WASTE_3                 ; 26
        ____SPKR_DUTY____4      ; 30
        WASTE_9                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid22:
        stx     next+1          ; 20
vs22b:  lda     $C099           ; 24
        and     #HAS_BYTE       ; 26
        ____SPKR_DUTY____4      ; 30
        beq     no_vid22b       ; 32/33
        WASTE_3                 ; 35
vd22b:  lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid22b:
ad22b:  ldx     $C0A8           ; 37
        stx     next+1          ; 40
        KBD_LOAD_13             ; 53
        WASTE_24                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1700, error
duty_cycle23:                    ; end spkr at 31
        ____SPKR_DUTY____4      ; 4
ad23:   ldx     $C0A8           ; 8
vs23:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid23        ; 16/17
vd23:   lda     $C098           ; 20
        stx     next+1          ; 23
        WASTE_4                 ; 27
        ____SPKR_DUTY____4      ; 31
        WASTE_8                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid23:
        stx     next+1          ; 20
vs23b:  lda     $C099           ; 24
        and     #HAS_BYTE       ; 26
        ____SPKR_DUTY____5      ; 31
        WASTE_2                 ; 33
        beq     no_vid23b       ; 35/36
vd23b:  lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid23b:
ad23b:  ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83


.align 256
.assert * = _SAMPLES_BASE + $1800, error
duty_cycle24:                    ; end spkr at 32
        ____SPKR_DUTY____4      ; 4
ad24:   ldx     $C0A8           ; 8
vs24:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid24        ; 16/17
vd24:   lda     $C098           ; 20
        stx     next+1          ; 23
        WASTE_5                 ; 28
        ____SPKR_DUTY____4      ; 32
        WASTE_7                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid24:
        WASTE_2                 ; 19
vs24b:  lda     $C099           ; 23
        and     #HAS_BYTE       ; 25
        beq     no_vid24b       ; 27/28
        ____SPKR_DUTY____5      ; 32
        stx     next+1          ; 35
vd24b:  lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid24b:
        ____SPKR_DUTY____4      ; 32
ad24b:  ldx     $C0A8           ; 36
        stx     next+1          ; 39
        KBD_LOAD_13             ; 52
        WASTE_25                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1900, error
duty_cycle25:                    ; end spkr at 33
        ____SPKR_DUTY____4      ; 4
ad25:   ldx     $C0A8           ; 8
vs25:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid25        ; 16/17
vd25:   lda     $C098           ; 20
        stx     next+1          ; 23
        WASTE_6                 ; 29
        ____SPKR_DUTY____4      ; 33
        WASTE_6                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid25:
        stx     next+1          ; 20
vs25b:  lda     $C099           ; 24
        and     #HAS_BYTE       ; 26
        beq     no_vid25b       ; 28/29
        ____SPKR_DUTY____5      ; 33
        WASTE_2                 ; 35
vd25b:  lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid25b:
        ____SPKR_DUTY____4      ; 33
ad25b:  ldx     $C0A8           ; 37
        stx     next+1          ; 40
        KBD_LOAD_13             ; 53
        WASTE_24                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1A00, error
duty_cycle26:                    ; end spkr at 34
        ____SPKR_DUTY____4      ; 4
ad26:   ldx     $C0A8           ; 8
vs26:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid26        ; 16/17
vd26:   lda     $C098           ; 20
        stx     next+1          ; 23
        WASTE_7                 ; 30
        ____SPKR_DUTY____4      ; 34
        WASTE_5                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid26:
vs26b:  lda     $C099           ; 21
        and     #HAS_BYTE       ; 23
        beq     no_vid26b       ; 25/26
vd26b:  lda     $C098           ; 29
        ____SPKR_DUTY____5      ; 34
        WASTE_2                 ; 36
        stx     next+1          ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid26b:
        WASTE_4                 ; 30
        ____SPKR_DUTY____4      ; 34
ad26b:  ldx     $C0A8           ; 38
        stx     next+1          ; 41
        KBD_LOAD_13             ; 54
        WASTE_23                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1B00, error
duty_cycle27:                    ; end spkr at 35
        ____SPKR_DUTY____4      ; 4
ad27:   ldx     $C0A8           ; 8
vs27:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid27        ; 16/17
vd27:   lda     $C098           ; 20
        stx     next+1          ; 23
        WASTE_8                 ; 31
        ____SPKR_DUTY____4      ; 35
        WASTE_4                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid27:
        stx     next+1          ; 20
        WASTE_2                 ; 22
vs27b:  lda     $C099           ; 26
        and     #HAS_BYTE       ; 28
        beq     no_vid27b       ; 30/31
        ____SPKR_DUTY____5      ; 35
vd27b:  lda     $C098           ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid27b:
        ____SPKR_DUTY____4      ; 35
ad27b:  ldx     $C0A8           ; 39
        stx     next+1          ; 42
        KBD_LOAD_13             ; 55
        WASTE_22                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1C00, error
duty_cycle28:                    ; end spkr at 36
        ____SPKR_DUTY____4      ; 4
ad28:   ldx     $C0A8           ; 8
vs28:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid28        ; 16/17
vd28:   lda     $C098           ; 20
        stx     next+1          ; 23
        WASTE_9                 ; 32
        ____SPKR_DUTY____4      ; 36
        WASTE_3                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid28:
        WASTE_2                 ; 19
vs28b:  lda     $C099           ; 23
        and     #HAS_BYTE       ; 25
        beq     no_vid28b       ; 27/28
vd28b:  lda     $C098           ; 31
        ____SPKR_DUTY____5      ; 36
        stx     next+1          ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid28b:
        WASTE_4                 ; 32
        ____SPKR_DUTY____4      ; 36
ad28b:  ldx     $C0A8           ; 40
        stx     next+1          ; 43
        KBD_LOAD_13             ; 56
        WASTE_21                ; 77
        jmp     (next)          ; 83


.align 256
.assert * = _SAMPLES_BASE + $1D00, error
duty_cycle29:                    ; end spkr at 37
        ____SPKR_DUTY____4      ; 4
ad29:   ldx     $C0A8           ; 8
vs29:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid29        ; 16/17
vd29:   lda     $C098           ; 20
        stx     next+1          ; 23
        WASTE_10                ; 33
        ____SPKR_DUTY____4      ; 37
        WASTE_2                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid29:
        stx     next+1          ; 20
vs29b:  lda     $C099           ; 24
        and     #HAS_BYTE       ; 26
        beq     no_vid29b       ; 28/29
vd29b:  lda     $C098           ; 32
        ____SPKR_DUTY____5      ; 37
        WASTE_2                 ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid29b:
        WASTE_4                 ; 33
        ____SPKR_DUTY____4      ; 37
ad29b:  ldx     $C0A8           ; 41
        stx     next+1          ; 44
        KBD_LOAD_13             ; 57
        WASTE_20                ; 77
        jmp     (next)          ; 83

.align 256
.assert * = _SAMPLES_BASE + $1E00, error
duty_cycle30:                    ; end spkr at 38
        ____SPKR_DUTY____4      ; 4
ad30:   ldx     $C0A8           ; 8
vs30:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid30        ; 16/17
vd30:   lda     $C098           ; 20
        WASTE_11                ; 31
        jmp     video_tog_spkr  ; 34=>83 (turns spkr off, jumps to next)

no_vid30:
        jmp     duty_cycle30_v2 ; 20=>83 (takes 63 cycles)

no_vid30b:
        WASTE_2                 ; 34
        ____SPKR_DUTY____4      ; 38
ad30b:  ldx     $C0A8           ; 42
        stx     next+1          ; 45
        KBD_LOAD_13             ; 58
        WASTE_19                ; 77
        jmp     (next)          ; 83

duty_cycle30_v2:
vs30b:  lda     $C099           ; 24
        and     has_byte_zp     ; 27
        bne     vd30b           ; 29/30
        jmp     no_vid30b       ; 32

vd30b:  lda     $C098           ; 34

; -----------------------------------------------------------------
video_tog_spkr:
        ____SPKR_DUTY____4      ; 38
        ABS_STX next+1          ; 42

video:
video_direct:
        bmi     control           ; 2/3
set_pixel:
        ldy     last_base         ; 5 Finish storing base
        tax                       ; 7
page_ptr_a:
        lda     (page1_addr_ptr),y; 12
        sta     store_dest+1      ; 15
        txa                       ; 17
        ldy     last_offset       ; 20
        sta     (store_dest),y    ; 26
        iny                       ; 28
        sty     last_offset       ; 31
        ABS_STZ got_offset        ; 35 - stz ABSOLUTE to waste one cycle
        jmp     (next)            ; 41

control:
        cmp     #$FF            ; 5
        beq     toggle_page     ; 7/8
        and     #%01111111      ; 9

        ; offset or base?
        ldx     got_offset      ; 12
        bne     set_base        ; 14/15

set_offset:
        sta     last_offset       ; 17
        inc     got_offset        ; 22
adv:    ldx     $C0A8             ; 26
        stx     next+1            ; 29
        WASTE_6                   ; 35
        jmp     (next)            ; 41

set_base:
        asl     a                 ; 17
        tay                       ; 19
page_ptr_b:
        lda     (page1_addr_ptr),y; 24
        sta     store_dest        ; 27
        iny                       ; 29
        sty     last_base         ; 32
        stz     got_offset        ; 35
        jmp     (next)            ; 41

toggle_page:
        ldx     page              ; 11
        sta     $C054,x           ; 16
        lda     page_addr_ptr,x   ; 20
        sta     page_ptr_a+1      ; 24
        sta     page_ptr_b+1      ; 28
        txa                       ; 30
        eor     #$01              ; 32 Toggle page for next time
        sta     page              ; 35
        jmp     (next)            ; 41
; -----------------------------------------------------------------

.align 256
.assert * = _SAMPLES_BASE + $1F00, error
duty_cycle31:                    ; end spkr at 39
        ____SPKR_DUTY____4      ; 4
ad31:   ldx     $C0A8           ; 8
vs31:   lda     $C099           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid31        ; 16/17
vd31:   lda     $C098           ; 20
        stx     next+1          ; 23
        WASTE_12                ; 35
        ____SPKR_DUTY____4      ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid31:
        stx     next+1          ; 20
vs31b:  lda     $C099           ; 24
        and     #HAS_BYTE       ; 26
        beq     no_vid31b       ; 28/29
vd31b:  lda     $C098           ; 32
        WASTE_2                 ; 34
        ____SPKR_DUTY____5      ; 39
        jmp     video_direct    ; 42=>83 (takes 41 cycles, jumps to next)

no_vid31b:
        WASTE_6                 ; 35
        ____SPKR_DUTY____4      ; 39
ad31b:  ldx     $C0A8           ; 43
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
