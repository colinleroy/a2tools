;
; Colin Leroy-Mira, 2024
;
        .export         _pwm
        .export         _SAMPLES_BASE

        .importzp       _zp6, _zp8, _zp10, _zp12, tmp1, ptr1

        .import         _serial_putc_direct
        .import         _simple_serial_set_irq
        .import         _simple_serial_flush
        .import         popa, VTABZ

.ifdef IIGS
        .import         zilog_status_reg_r, zilog_data_reg_r
        .import         _get_iigs_speed, _set_iigs_speed
        .include        "accelerator.inc"
.else
        .import         acia_status_reg_r, acia_data_reg_r
.endif

        .include        "apple2.inc"

; ------------------------------------------------------------------------

.ifdef IIGS
serial_status_reg = zilog_status_reg_r
serial_data_reg   = zilog_data_reg_r
HAS_BYTE          = $01
.else
serial_status_reg = acia_status_reg_r
serial_data_reg   = acia_data_reg_r
HAS_BYTE          = $08
.endif

SPKR         := $C030

data         := $FFFF           ; Placeholders for legibility, going to be patched
status       := $FFFF

spkr_ptr      = _zp6
dummy_ptr     = _zp8
status_ptr    = _zp10
data_ptr      = _zp12
dummy_zp      = tmp1

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

.ifdef __APPLE2ENH__
  .macro WASTE_5
        sta     (dummy_ptr)
  .endmacro
.else
  .macro WASTE_5
        WASTE_2
        WASTE_3
  .endmacro
.endif

.macro WASTE_6
        inc     dummy_abs
.endmacro

.macro WASTE_7
        WASTE_2
        WASTE_5
.endmacro
.macro WASTE_8
        WASTE_2
        WASTE_6
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
        WASTE_2
        WASTE_11
.endmacro
.macro WASTE_14
        WASTE_2
        WASTE_12
.endmacro
.macro WASTE_15
        WASTE_3
        WASTE_12
.endmacro
.macro WASTE_16
        WASTE_4
        WASTE_12
.endmacro
.macro WASTE_17
        WASTE_5
        WASTE_12
.endmacro
.macro WASTE_18
        WASTE_6
        WASTE_12
.endmacro

.macro KBD_LOAD_7               ; Check keyboard and jsr if key pressed (trashes A)
        lda     KBD             ; 4
        bpl     :+              ; 7
        jsr     kbd_send
:
.endmacro

.macro ____SPKR_DUTY____4       ; Toggle speaker
        sta     SPKR            ; 4
.endmacro

.ifdef __APPLE2ENH__
  .macro ____SPKR_DUTY____5       ; Toggle speaker slower
          sta     (spkr_ptr)      ; 5
  .endmacro
.endif

.macro SER_AVAIL_A_6            ; Get byte availability to A
        lda     status          ; 4
        and     #HAS_BYTE       ; 6
.endmacro

.macro SER_LOOP_IF_NOT_AVAIL_2  ; Loop if no byte available
        beq     :+              ; 2 / 3
.endmacro

.macro SER_FETCH_DEST_A_4       ; Fetch next duty cycle to A
        lda     data            ; 4
.endmacro
        .code

.align 256
_SAMPLES_BASE = *
.assert * = $4000, error
duty_cycle0:                    ; Max negative level, 8 cycles
        ____SPKR_DUTY____4      ; 4  !
        ____SPKR_DUTY____4      ; 8  !

        WASTE_11                ; 19
        KBD_LOAD_7              ; 26

s0:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d0:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest0+2         ; 42
dest0:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle0     ;    45

; -----------------------------------------------------------------
kbd_send:
        and     #$7F            ; Clear high bit
        bit     KBDSTRB         ; Clear keyboard strobe
        cmp     #' '
        beq     pause
        jmp     _serial_putc_direct
pause:
        jsr     _serial_putc_direct
:       lda     KBD
        bpl     :-
        and     #$7F            ; Clear high bit
        bit     KBDSTRB         ; Clear keyboard strobe
        jmp     _serial_putc_direct
; ------------------------------------------------------------------
silence:
ssil:   lda     status
        and     #HAS_BYTE
        beq     silence
dsil:   lda     data
        sta     start_duty+2
start_duty:
        jmp     $0000
; ------------------------------------------------------------------

.align 256
.assert * = _SAMPLES_BASE+$100, error
duty_cycle1:
        ____SPKR_DUTY____4      ; 4  !
        .ifdef __APPLE2ENH__
        ____SPKR_DUTY____5      ; 9  !
        WASTE_2                 ; 11
        .else
        ____SPKR_DUTY____4      ; 8  ! Approximation here
        WASTE_3                 ; 11
        .endif

        WASTE_8                 ; 19
        KBD_LOAD_7              ; 26

s1:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d1:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest1+2         ; 42
dest1:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle1     ;    45

; ------------------------------------------------------------------
setup_pointers:
        ; Setup pointer access to SPKR
        lda     #<(SPKR)
        sta     spkr_ptr
        lda     #>(SPKR)
        sta     spkr_ptr+1

        ; Setup pointer access to dummy var
        lda     #<(dummy_abs)
        sta     dummy_ptr
        lda     #>(dummy_abs)
        sta     dummy_ptr+1
        rts
; ------------------------------------------------------------------

.align 256
.assert * = _SAMPLES_BASE+$200, error
duty_cycle2:
        ____SPKR_DUTY____4      ; 4 !
        WASTE_2                 ; 6 !
        ____SPKR_DUTY____4      ; 10 !

        WASTE_16                ; 26

s2:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d2:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest2+2         ; 42
dest2:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle2     ;    45

; ------------------------------------------------------------------
patch_status_register:
        ; Patch ALL the serial regs!
        ; Status low byte
        lda     serial_status_reg+1
        sta     status_ptr
        sta     s0+1
        sta     s1+1
        sta     s2+1
        sta     s3+1
        sta     s4+1
        sta     s5+1
        sta     s6+1
        sta     s7+1
        sta     s8+1
        sta     s9+1
        sta     s10+1
        sta     s11+1
        sta     s12+1
        sta     s13+1
        sta     s14+1
        sta     s15+1
        sta     s16+1
        sta     s17+1
        sta     s18+1
        sta     s19+1
        sta     s20+1
        sta     s21+1
        sta     s22+1
        sta     s23+1
        sta     s24+1
        sta     s25+1
        sta     s26+1
        sta     s27+1
        sta     s28+1
        sta     s29+1
        sta     s30+1
        sta     s31+1
        sta     s32+1
        sta     ssil+1

        ; Status high byte
        lda     serial_status_reg+2
        sta     status_ptr+1
        sta     s0+2
        sta     s1+2
        sta     s2+2
        sta     s3+2
        sta     s4+2
        sta     s5+2
        sta     s6+2
        sta     s7+2
        sta     s8+2
        sta     s9+2
        sta     s10+2
        sta     s11+2
        sta     s12+2
        sta     s13+2
        sta     s14+2
        sta     s15+2
        sta     s16+2
        sta     s17+2
        sta     s18+2
        sta     s19+2
        sta     s20+2
        sta     s21+2
        sta     s22+2
        sta     s23+2
        sta     s24+2
        sta     s25+2
        sta     s26+2
        sta     s27+2
        sta     s28+2
        sta     s29+2
        sta     s30+2
        sta     s31+2
        sta     s32+2
        sta     ssil+2
        rts
; ------------------------------------------------------------------

.align 256
.assert * = _SAMPLES_BASE+$300, error
duty_cycle3:
        ____SPKR_DUTY____4      ; 4 !
        WASTE_3                 ; 7 !
        ____SPKR_DUTY____4      ; 11 !

        WASTE_15                ; 26

s3:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d3:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest3+2         ; 42
dest3:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle3     ;    45

; ------------------------------------------------------------------
patch_data_register:
        ; Data low byte
        lda     serial_data_reg+1
        sta     data_ptr
        sta     d0+1
        sta     d1+1
        sta     d2+1
        sta     d3+1
        sta     d4+1
        sta     d5+1
        sta     d6+1
        sta     d7+1
        sta     d8+1
        sta     d9+1
        sta     d10+1
        sta     d11+1
        sta     d12+1
        sta     d13+1
        sta     d14+1
        sta     d15+1
        sta     d16+1
        sta     d17+1
        sta     d18+1
        sta     d19+1
        sta     d20+1
        sta     d21+1
        sta     d22+1
        sta     d23+1
        sta     d24+1
        sta     d25+1
        sta     d26+1
        sta     d27+1
        sta     d28+1
        sta     d29+1
        sta     d30+1
        sta     d31+1
        sta     d32+1
        sta     dsil+1

        ; Data high byte
        lda     serial_data_reg+2
        sta     data_ptr+1
        sta     d0+2
        sta     d1+2
        sta     d2+2
        sta     d3+2
        sta     d4+2
        sta     d5+2
        sta     d6+2
        sta     d7+2
        sta     d8+2
        sta     d9+2
        sta     d10+2
        sta     d11+2
        sta     d12+2
        sta     d13+2
        sta     d14+2
        sta     d15+2
        sta     d16+2
        sta     d17+2
        sta     d18+2
        sta     d19+2
        sta     d20+2
        sta     d21+2
        sta     d22+2
        sta     d23+2
        sta     d24+2
        sta     d25+2
        sta     d26+2
        sta     d27+2
        sta     d28+2
        sta     d29+2
        sta     d30+2
        sta     d31+2
        sta     d32+2
        sta     dsil+2
        rts
; ------------------------------------------------------------------

.align 256
.assert * = _SAMPLES_BASE+$400, error
duty_cycle4:
        ____SPKR_DUTY____4      ; 4 !
        WASTE_4                 ; 8 !
        ____SPKR_DUTY____4      ; 12 !

        KBD_LOAD_7              ; 19
        WASTE_7                 ; 26

s4:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d4:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest4+2         ; 42
dest4:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle4     ;    45


.align 256
.assert * = _SAMPLES_BASE+$500, error
duty_cycle5:
        ____SPKR_DUTY____4      ; 4 !
        WASTE_5                 ; 9 !
        ____SPKR_DUTY____4      ; 13 !

        WASTE_6                 ; 19
        KBD_LOAD_7              ; 26

s5:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d5:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest5+2         ; 42
dest5:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle5     ;    45


.align 256
.assert * = _SAMPLES_BASE+$600, error
duty_cycle6:
        ____SPKR_DUTY____4      ; 4 !
        WASTE_6                 ; 10 !
        ____SPKR_DUTY____4      ; 14 !

        WASTE_5                 ; 19
        KBD_LOAD_7              ; 26

s6:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d6:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest6+2         ; 42
dest6:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle6     ;    45


.align 256
.assert * = _SAMPLES_BASE+$700, error
duty_cycle7:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        ____SPKR_DUTY____4      ; 15 !

        WASTE_11                ; 26

s7:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d7:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest7+2         ; 42
dest7:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle7     ;    45


.align 256
.assert * = _SAMPLES_BASE+$800, error
duty_cycle8:
        ____SPKR_DUTY____4      ; 4 !
        WASTE_8                 ; 12 !
        ____SPKR_DUTY____4      ; 16 !

        WASTE_3                 ; 19
        KBD_LOAD_7              ; 26

s8:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d8:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest8+2         ; 42
dest8:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle8     ;    45


.align 256
.assert * = _SAMPLES_BASE+$900, error
duty_cycle9:
        ____SPKR_DUTY____4      ; 4 !
        WASTE_2                 ; 6 !
        KBD_LOAD_7              ; 13 !
        ____SPKR_DUTY____4      ; 17 !

        WASTE_9                ; 26

s9:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d9:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest9+2         ; 42
dest9:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle9     ;    45


.align 256
.assert * = _SAMPLES_BASE+$A00, error
duty_cycle10:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_3                 ; 14 !
        ____SPKR_DUTY____4      ; 18 !

        WASTE_8                 ; 26

s10:    SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d10:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest10+2        ; 42
dest10:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle10    ;    45


.align 256
.assert * = _SAMPLES_BASE+$B00, error
duty_cycle11:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_4                 ; 15 !
        ____SPKR_DUTY____4      ; 19 !

        WASTE_7                 ; 26

s11:    SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d11:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest11+2        ; 42
dest11:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle11    ;    45


.align 256
.assert * = _SAMPLES_BASE+$C00, error
duty_cycle12:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_5                 ; 16 !
        ____SPKR_DUTY____4      ; 20 !

        WASTE_6                 ; 26

s12:    SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d12:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest12+2        ; 42
dest12:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle12    ;    45


.align 256
.assert * = _SAMPLES_BASE+$D00, error
duty_cycle13:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_6                 ; 17 !
        ____SPKR_DUTY____4      ; 21 !

        WASTE_5                 ; 26

s13:    SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d13:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest13+2        ; 42
dest13:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle13    ;    45

.align 256
.assert * = _SAMPLES_BASE+$E00, error
duty_cycle14:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_7                 ; 18 !
        ____SPKR_DUTY____4      ; 22 !

        WASTE_4                 ; 26

s14:    SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d14:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest14+2        ; 42
dest14:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle14    ;    45


.align 256
.assert * = _SAMPLES_BASE+$F00, error
duty_cycle15:                   ; Middle level, zero, 23 cycles
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_8                 ; 19 !
        ____SPKR_DUTY____4      ; 23 !

        WASTE_3                 ; 26

s15:    SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d15:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest15+2        ; 42
dest15:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle15    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1000, error
duty_cycle16:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_9                 ; 19 !
        ____SPKR_DUTY____4      ; 24 !

        WASTE_2                 ; 26

s16:    SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d16:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest16+2        ; 42
dest16:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle16    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1100, error
duty_cycle17:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_8                 ; 19 !
        ldx     #$00            ; 21 !
        ____SPKR_DUTY____4      ; 25 !

s17:    SER_AVAIL_A_6           ; 31
        SER_LOOP_IF_NOT_AVAIL_2 ; 33 34
d17:    SER_FETCH_DEST_A_4      ; 37      - yes
        sta     dest17+2,x      ; 42
dest17:
        jmp     $0000           ; 45
:
        WASTE_8                 ;    42
        jmp     duty_cycle17    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1200, error
duty_cycle18:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_11                ; 22 !
        ____SPKR_DUTY____4      ; 26 !

s18:    SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d18:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest18+2        ; 42
dest18:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle18    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1300, error
duty_cycle19:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_6                 ; 17 !
s19:    SER_AVAIL_A_6           ; 23 !
        ____SPKR_DUTY____4      ; 27 !

        WASTE_5                 ; 32

        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d19:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest19+2        ; 42
dest19:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle19    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1400, error
duty_cycle20:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_7                 ; 18 !
s20:    SER_AVAIL_A_6           ; 24 !
        ____SPKR_DUTY____4      ; 28 !

        WASTE_4                 ; 32

        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d20:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest20+2        ; 42
dest20:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle20    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1500, error
duty_cycle21:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_8                 ; 19 !
s21:    SER_AVAIL_A_6           ; 25 !
        ____SPKR_DUTY____4      ; 29 !

        WASTE_3                 ; 32

        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d21:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest21+2        ; 42
dest21:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle21    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1600, error
duty_cycle22:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_9                 ; 20 !
s22:    SER_AVAIL_A_6           ; 26 !
        ____SPKR_DUTY____4      ; 30 !

        WASTE_2                 ; 32

        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d22:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest22+2        ; 42
dest22:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle22    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1700, error
duty_cycle23:
        ____SPKR_DUTY____4      ; 4 !
        ldx     #$00            ; 6 !
        KBD_LOAD_7              ; 13 !
        WASTE_8                 ; 21 !
s23:    SER_AVAIL_A_6           ; 27 !
        ____SPKR_DUTY____4      ; 31 !

        SER_LOOP_IF_NOT_AVAIL_2 ; 33 35
d23:    SER_FETCH_DEST_A_4      ; 37      - yes
        sta     dest23+2,x      ; 42
dest23:
        jmp     $0000           ; 45
:
        WASTE_8                 ;    42
        jmp     duty_cycle23    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1800, error
duty_cycle24:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_11                ; 22 !
s24:    SER_AVAIL_A_6           ; 28 !
        ____SPKR_DUTY____4      ; 32 !

        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d24:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest24+2        ; 42
dest24:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    42
        jmp     duty_cycle24    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1900, error
duty_cycle25:
        ____SPKR_DUTY____4      ; 4 !
        ldx     #$00            ; 6 !
        KBD_LOAD_7              ; 13 !
        WASTE_5                 ; 18 !
s25:    SER_AVAIL_A_6           ; 24 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 26 ! 27
        WASTE_3                 ; 29 !
        ____SPKR_DUTY____4      ; 33 !

d25:    SER_FETCH_DEST_A_4      ; 37      - yes
        sta     dest25+2,x      ; 42
dest25:
        jmp     $0000           ; 45
:
        WASTE_2                 ;    29 !
        ____SPKR_DUTY____4      ;    33 !
        WASTE_9                 ;    42
        jmp     duty_cycle25    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1A00, error
duty_cycle26:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_8                 ; 19 !
s26:    SER_AVAIL_A_6           ; 25 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 27 ! 28
        WASTE_3                 ; 30 !
        ____SPKR_DUTY____4      ; 34 !

d26:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest26+2        ; 42
dest26:
        jmp     $0000           ; 45
:
        WASTE_2                 ;    30 !
        ____SPKR_DUTY____4      ;    34 !
        WASTE_8                 ;    42
        jmp     duty_cycle26    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1B00, error
duty_cycle27:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_8                 ; 19 !
s27:    SER_AVAIL_A_6           ; 25 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 27 ! 28
d27:    SER_FETCH_DEST_A_4      ; 31 !     - yes
        ____SPKR_DUTY____4      ; 35 !

        WASTE_3                 ; 38
        sta     dest27+2        ; 42
dest27:
        jmp     $0000           ; 45
:
        WASTE_3                 ;    31 !
        ____SPKR_DUTY____4      ;    35 !
        WASTE_7                 ;    42
        jmp     duty_cycle27    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1C00, error
duty_cycle28:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_9                 ; 20 !
s28:    SER_AVAIL_A_6           ; 26 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 28 ! 29
d28:    SER_FETCH_DEST_A_4      ; 32 !
        ____SPKR_DUTY____4      ; 36 !

        WASTE_2                 ; 38
        sta     dest28+2        ; 42
dest28:
        jmp     $0000           ; 45
:
        WASTE_3                 ;    32 !
        ____SPKR_DUTY____4      ;    36 !
        WASTE_6                 ;    42
        jmp     duty_cycle28    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1D00, error
duty_cycle29:
        ____SPKR_DUTY____4      ; 4 !
        ldx     #$00            ; 6 !
        KBD_LOAD_7              ; 13 !
        WASTE_8                 ; 21 !
s29:    SER_AVAIL_A_6           ; 27 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 29 ! 30
d29:    SER_FETCH_DEST_A_4      ; 33 !
        ____SPKR_DUTY____4      ; 37 !

        sta     dest29+2,x      ; 42
dest29:
        jmp     $0000           ; 45
:
        WASTE_3                 ;    33 !
        ____SPKR_DUTY____4      ;    37 !
        WASTE_5                 ;    42
        jmp     duty_cycle29    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1E00, error
duty_cycle30:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_9                 ; 20 !
s30:    SER_AVAIL_A_6           ; 26 !
        WASTE_2                 ; 28 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 30 ! 31
d30:    SER_FETCH_DEST_A_4      ; 34 !
        ____SPKR_DUTY____4      ; 38 !

        sta     dest30+2        ; 42
dest30:
        jmp     $0000           ; 45
:
        WASTE_3                 ;    34 !
        ____SPKR_DUTY____4      ;    38 !
        WASTE_4                 ;    42
        jmp     duty_cycle30    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1F00, error
duty_cycle31:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_8                 ; 19 !
s31:    SER_AVAIL_A_6           ; 25 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 27 ! 28
d31:    SER_FETCH_DEST_A_4      ; 31 !
        sta     dest31+2        ; 35 !
        ____SPKR_DUTY____4      ; 39 !
        WASTE_3                 ; 42
dest31:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    35 !
        ____SPKR_DUTY____4      ;    39 !
        WASTE_3                 ;    42
        jmp     duty_cycle31    ;    45

.align 256
.assert * = _SAMPLES_BASE+$2000, error
duty_cycle32:                   ; Max positive level, 40 cycles
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_9                 ; 20 !
s32:    SER_AVAIL_A_6           ; 26 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 28 ! 29
d32:    SER_FETCH_DEST_A_4      ; 32 !
        sta     dest32+2        ; 36 !
        ____SPKR_DUTY____4      ; 40 !
        WASTE_2                 ; 42
dest32:
        jmp     $0000           ; 45
:
        WASTE_7                 ;    36 !
        ____SPKR_DUTY____4      ;    40 !
        WASTE_2                 ;    42
        jmp     duty_cycle32    ;    45

.align 256
.assert * = _SAMPLES_BASE+$2100, error
break_out:
.ifdef IIGS
        lda     prevspd
        jsr     _set_iigs_speed
.endif
        lda     #$01            ; Reenable IRQ and flush
        jsr     _simple_serial_set_irq
        jmp     _simple_serial_flush

_pwm:
        pha
        ; Disable interrupts
        lda     #$00
        jsr     _simple_serial_set_irq

        pla
        ; Setup pointers
        jsr     setup_pointers
        ; Patch serial registers
        jsr     patch_status_register
        jsr     patch_data_register

.ifdef IIGS
        ; Slow down IIgs
        jsr     _get_iigs_speed
        sta     prevspd
        lda     #SPEED_SLOW
        jsr     _set_iigs_speed
.endif
        ; Start with silence
        jmp     silence

        .bss
dummy_abs: .res 1
counter:   .res 1
prevspd:   .res 1
