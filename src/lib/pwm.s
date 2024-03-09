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
vumeter_base  = ptr1

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
        sta     dummy_abs
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

.macro VU_CLEAR_7               ; Clear previous VU meter level (trashes A)
        lda     #' '|$80        ; 2
        sta     (vumeter_base),y; 7
.endmacro

.macro VU_SET_9                     ; Set new VU meter level (trashes A)
        ldy     #>(*-_SAMPLES_BASE) ; 2
        lda     #' '                ; 4
        sta     (vumeter_base),y    ; 9
.endmacro

.macro VU_CLEAR_AND_SET_16      ; Clear and set VU meter (trashes A)
        VU_CLEAR_7
        VU_SET_9
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
duty_cycle0:
        ____SPKR_DUTY____4      ; 4  !
        ____SPKR_DUTY____4      ; 8  !

        KBD_LOAD_7              ; 15
        VU_CLEAR_AND_SET_16     ; 31

        WASTE_14                ; 45

        ____SPKR_DUTY____4      ; 49  !
        ____SPKR_DUTY____4      ; 53  !

        WASTE_18                ; 71

s0:     SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d0:     SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest0+2         ; 87
dest0:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle0     ;    90

.align 256
.assert * = _SAMPLES_BASE+$100, error
duty_cycle1:
        ____SPKR_DUTY____4      ; 4  !
        .ifdef __APPLE2ENH__
        ____SPKR_DUTY____5      ; 9  !
        WASTE_13                ; 22
        .else
        ____SPKR_DUTY____4      ; 8  ! Approximation here
        WASTE_14                ; 22
        .endif

        KBD_LOAD_7              ; 29
        VU_CLEAR_AND_SET_16     ; 45


        ____SPKR_DUTY____4      ; 49  !
        .ifdef __APPLE2ENH__
        ____SPKR_DUTY____5      ; 54  !
        WASTE_17                ; 71
        .else
        ____SPKR_DUTY____4      ; 53  ! Approximation here
        WASTE_18                ; 71
        .endif

s1:     SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d1:     SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest1+2         ; 87
dest1:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle1     ;    90

.align 256
.assert * = _SAMPLES_BASE+$200, error
duty_cycle2:
        ____SPKR_DUTY____4      ; 4  !
        WASTE_2                 ; 6  !
        ____SPKR_DUTY____4      ; 10 !

        KBD_LOAD_7              ; 17
        VU_CLEAR_AND_SET_16     ; 33

        WASTE_12                ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_2                 ; 51 !
        ____SPKR_DUTY____4      ; 55 !

        WASTE_16                ; 71

s2:     SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d2:     SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest2+2         ; 87
dest2:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle2     ;    90

.align 256
.assert * = _SAMPLES_BASE+$300, error
duty_cycle3:
        ____SPKR_DUTY____4      ; 4  !
        WASTE_3                 ; 7  !
        ____SPKR_DUTY____4      ; 11 !
        
        KBD_LOAD_7              ; 18
        VU_CLEAR_AND_SET_16     ; 34

        WASTE_11                ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_3                 ; 52 !
        ____SPKR_DUTY____4      ; 56 !

        WASTE_15                ; 71

s3:     SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d3:     SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest3+2         ; 87
dest3:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle3     ;    90


.align 256
.assert * = _SAMPLES_BASE+$400, error
duty_cycle4:
        ____SPKR_DUTY____4      ; 4  !
        WASTE_4                 ; 8  !
        ____SPKR_DUTY____4      ; 12 !

        KBD_LOAD_7              ; 19
        VU_CLEAR_AND_SET_16     ; 35

        WASTE_10                ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_4                 ; 53 !
        ____SPKR_DUTY____4      ; 57 !

        WASTE_14                ; 71

s4:     SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d4:     SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest4+2         ; 87
dest4:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle4     ;    90


.align 256
.assert * = _SAMPLES_BASE+$500, error
duty_cycle5:
        ____SPKR_DUTY____4      ; 4  !
        WASTE_5                 ; 9  !
        ____SPKR_DUTY____4      ; 13 !
        
        KBD_LOAD_7              ; 20
        VU_CLEAR_AND_SET_16     ; 36

        WASTE_9                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_5                 ; 54 !
        ____SPKR_DUTY____4      ; 58 !

        WASTE_13                ; 71

s5:     SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d5:     SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest5+2         ; 87
dest5:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle5     ;    90


.align 256
.assert * = _SAMPLES_BASE+$600, error
duty_cycle6:
        ____SPKR_DUTY____4      ; 4  !
        WASTE_6                 ; 10  !
        ____SPKR_DUTY____4      ; 14 !
        
        KBD_LOAD_7              ; 21
        VU_CLEAR_AND_SET_16     ; 37

        WASTE_8                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_6                 ; 55 !
        ____SPKR_DUTY____4      ; 59 !

        WASTE_12                ; 71

s6:     SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d6:     SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest6+2         ; 87
dest6:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle6     ;    90


.align 256
.assert * = _SAMPLES_BASE+$700, error
duty_cycle7:
        ____SPKR_DUTY____4      ; 4  !
        WASTE_7                 ; 11 !
        ____SPKR_DUTY____4      ; 15 !
        
        KBD_LOAD_7              ; 22
        VU_CLEAR_AND_SET_16     ; 38

        WASTE_7                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_7                 ; 56 !
        ____SPKR_DUTY____4      ; 60 !

        WASTE_11                ; 71

s7:     SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d7:     SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest7+2         ; 87
dest7:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle7     ;    90


.align 256
.assert * = _SAMPLES_BASE+$800, error
duty_cycle8:
        ____SPKR_DUTY____4      ; 4  !
        WASTE_8                 ; 12 !
        ____SPKR_DUTY____4      ; 16 !

        KBD_LOAD_7              ; 23
        VU_CLEAR_AND_SET_16     ; 39

        WASTE_6                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_8                 ; 57 !
        ____SPKR_DUTY____4      ; 61 !

        WASTE_10                ; 71

s8:     SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d8:     SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest8+2         ; 87
dest8:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle8     ;    90


.align 256
.assert * = _SAMPLES_BASE+$900, error
duty_cycle9:
        ____SPKR_DUTY____4      ; 4  !
        WASTE_9                 ; 13 !
        ____SPKR_DUTY____4      ; 17 !
        
        KBD_LOAD_7              ; 24
        VU_CLEAR_AND_SET_16     ; 40

        WASTE_5                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_9                 ; 58 !
        ____SPKR_DUTY____4      ; 62 !

        WASTE_9                 ; 71

s9:     SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d9:     SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest9+2         ; 87
dest9:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle9     ;    90


.align 256
.assert * = _SAMPLES_BASE+$A00, error
duty_cycle10:
        ____SPKR_DUTY____4      ; 4  !
        WASTE_10                ; 14 !
        ____SPKR_DUTY____4      ; 18 !
        
        KBD_LOAD_7              ; 25
        VU_CLEAR_AND_SET_16     ; 41

        WASTE_4                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_10                ; 59 !
        ____SPKR_DUTY____4      ; 63 !

        WASTE_8                 ; 71

s10:    SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d10:    SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest10+2        ; 87
dest10:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle10    ;    90


.align 256
.assert * = _SAMPLES_BASE+$B00, error
duty_cycle11:
        ____SPKR_DUTY____4      ; 4  !
        WASTE_11                ; 15 !
        ____SPKR_DUTY____4      ; 19 !
        
        KBD_LOAD_7              ; 26
        VU_CLEAR_AND_SET_16     ; 42

        WASTE_3                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_11                ; 60 !
        ____SPKR_DUTY____4      ; 64 !

        WASTE_7                 ; 71

s11:    SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d11:    SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest11+2        ; 87
dest11:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle11    ;    90


.align 256
.assert * = _SAMPLES_BASE+$C00, error
duty_cycle12:
        ____SPKR_DUTY____4      ; 4  !
        WASTE_12                ; 16 !
        ____SPKR_DUTY____4      ; 20 !
        
        KBD_LOAD_7              ; 27
        VU_CLEAR_AND_SET_16     ; 43

        WASTE_2                   ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_12                ; 61 !
        ____SPKR_DUTY____4      ; 65 !

        WASTE_6                 ; 71

s12:    SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d12:    SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest12+2        ; 87
dest12:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle12    ;    90


.align 256
.assert * = _SAMPLES_BASE+$D00, error
duty_cycle13:
        ____SPKR_DUTY____4      ; 4  !
        WASTE_6                 ; 10 !
        KBD_LOAD_7              ; 17 !
        ____SPKR_DUTY____4      ; 21 !
        
        VU_CLEAR_AND_SET_16     ; 37

        WASTE_8                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_13                ; 62 !
        ____SPKR_DUTY____4      ; 66 !

        WASTE_5                 ; 71

s13:    SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d13:    SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest13+2        ; 87
dest13:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle13    ;    90

.align 256
.assert * = _SAMPLES_BASE+$E00, error
duty_cycle14:
        ____SPKR_DUTY____4      ; 4  !
        WASTE_7                 ; 11 !
        KBD_LOAD_7              ; 18 !
        ____SPKR_DUTY____4      ; 22 !
        
        VU_CLEAR_AND_SET_16     ; 38

        WASTE_7                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_14                ; 63 !
        ____SPKR_DUTY____4      ; 67 !

        WASTE_4                 ; 71

s14:    SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d14:    SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest14+2        ; 87
dest14:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle14    ;    90


.align 256
.assert * = _SAMPLES_BASE+$F00, error
duty_cycle15:
        ____SPKR_DUTY____4      ; 4  !
        WASTE_8                 ; 12 !
        KBD_LOAD_7              ; 19 !
        ____SPKR_DUTY____4      ; 23 !
        
        VU_CLEAR_AND_SET_16     ; 39

        WASTE_6                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_15                ; 64 !
        ____SPKR_DUTY____4      ; 68 !

        WASTE_3                 ; 71

s15:    SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d15:    SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest15+2        ; 87
dest15:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle15    ;    90


.align 256
.assert * = _SAMPLES_BASE+$1000, error
duty_cycle16:
        ____SPKR_DUTY____4      ; 4  !
        WASTE_9                 ; 13 !
        KBD_LOAD_7              ; 20 !
        ____SPKR_DUTY____4      ; 24 !

        VU_CLEAR_AND_SET_16     ; 40

        WASTE_5                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_16                ; 65 !
        ____SPKR_DUTY____4      ; 69 !

        WASTE_2                 ; 71

s16:    SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d16:    SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest16+2        ; 87
dest16:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle16    ;    90


.align 256
.assert * = _SAMPLES_BASE+$1100, error
duty_cycle17:
        ____SPKR_DUTY____4      ; 4  !
        WASTE_10                ; 14 !
        KBD_LOAD_7              ; 21 !
        ____SPKR_DUTY____4      ; 25 !
        
        VU_CLEAR_AND_SET_16     ; 41

        WASTE_4                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_15                ; 64 !
        ldx     #$00            ; 66 !
        ____SPKR_DUTY____4      ; 70 !

s17:    SER_AVAIL_A_6           ; 76
        SER_LOOP_IF_NOT_AVAIL_2 ; 78 79
d17:    SER_FETCH_DEST_A_4      ; 82      - yes
        sta     dest17+2,x      ; 87
dest17:
        jmp     $0000           ; 90
:
        WASTE_8                 ;    87
        jmp     duty_cycle17    ;    90


.align 256
.assert * = _SAMPLES_BASE+$1200, error
duty_cycle18:
        ____SPKR_DUTY____4      ; 4  !
        VU_CLEAR_AND_SET_16     ; 20 !
        WASTE_2                 ; 22 !
        ____SPKR_DUTY____4      ; 26 !
        
        KBD_LOAD_7              ; 33
        WASTE_12                ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_18                ; 67 !
        ____SPKR_DUTY____4      ; 71 !

s18:    SER_AVAIL_A_6           ; 77
        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d18:    SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest18+2        ; 87
dest18:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle18    ;    90


.align 256
.assert * = _SAMPLES_BASE+$1300, error
duty_cycle19:
        ____SPKR_DUTY____4      ; 4  !
        VU_CLEAR_AND_SET_16     ; 20 !
        WASTE_3                 ; 23 !
        ____SPKR_DUTY____4      ; 27 !
        
        KBD_LOAD_7              ; 34
        WASTE_11                ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_13                ; 62 !
s19:    SER_AVAIL_A_6           ; 68 !
        ____SPKR_DUTY____4      ; 72 !

        WASTE_5                 ; 77

        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d19:    SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest19+2        ; 87
dest19:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle19    ;    90


.align 256
.assert * = _SAMPLES_BASE+$1400, error
duty_cycle20:
        ____SPKR_DUTY____4      ; 4  !
        VU_CLEAR_AND_SET_16     ; 20 !
        WASTE_4                 ; 24 !
        ____SPKR_DUTY____4      ; 28 !

        KBD_LOAD_7              ; 35
        WASTE_10                ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_14                ; 63 !
s20:    SER_AVAIL_A_6           ; 69 !
        ____SPKR_DUTY____4      ; 73 !

        WASTE_4                 ; 77

        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d20:    SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest20+2        ; 87
dest20:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle20    ;    90


.align 256
.assert * = _SAMPLES_BASE+$1500, error
duty_cycle21:
        ____SPKR_DUTY____4      ; 4  !
        VU_CLEAR_AND_SET_16     ; 20 !
        WASTE_5                 ; 25 !
        ____SPKR_DUTY____4      ; 29 !
        
        KBD_LOAD_7              ; 36
        WASTE_9                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_15                ; 64 !
s21:    SER_AVAIL_A_6           ; 70 !
        ____SPKR_DUTY____4      ; 74 !

        WASTE_3                 ; 77

        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d21:    SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest21+2        ; 87
dest21:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle21    ;    90


.align 256
.assert * = _SAMPLES_BASE+$1600, error
duty_cycle22:
        ____SPKR_DUTY____4      ; 4  !
        VU_CLEAR_AND_SET_16     ; 20 !
        WASTE_6                 ; 26 !
        ____SPKR_DUTY____4      ; 30 !
        
        KBD_LOAD_7              ; 37
        WASTE_8                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_16                ; 65 !
s22:    SER_AVAIL_A_6           ; 71 !
        ____SPKR_DUTY____4      ; 75 !

        WASTE_2                 ; 77

        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d22:    SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest22+2        ; 87
dest22:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle22    ;    90


.align 256
.assert * = _SAMPLES_BASE+$1700, error
duty_cycle23:
        ____SPKR_DUTY____4      ; 4  !
        VU_CLEAR_AND_SET_16     ; 20 !
        WASTE_7                 ; 27 !
        ____SPKR_DUTY____4      ; 31 !
        
        KBD_LOAD_7              ; 38
        WASTE_7                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        ldx     #$00            ; 51 !
        WASTE_15                ; 66 !
s23:    SER_AVAIL_A_6           ; 72 !
        ____SPKR_DUTY____4      ; 76 !

        SER_LOOP_IF_NOT_AVAIL_2 ; 78 79
d23:    SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest23+2,x      ; 87
dest23:
        jmp     $0000           ; 90
:
        WASTE_8                 ;    87
        jmp     duty_cycle23    ;    90


.align 256
.assert * = _SAMPLES_BASE+$1800, error
duty_cycle24:
        ____SPKR_DUTY____4      ; 4  !
        VU_CLEAR_AND_SET_16     ; 20 !
        WASTE_8                 ; 28 !
        ____SPKR_DUTY____4      ; 32 !
        
        KBD_LOAD_7              ; 39
        WASTE_6                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_18                ; 67 !
s24:    SER_AVAIL_A_6           ; 73 !
        ____SPKR_DUTY____4      ; 77 !

        SER_LOOP_IF_NOT_AVAIL_2 ; 79 80
d24:    SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest24+2        ; 87
dest24:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    87
        jmp     duty_cycle24    ;    90


.align 256
.assert * = _SAMPLES_BASE+$1900, error
duty_cycle25:
        ____SPKR_DUTY____4      ; 4  !
        VU_CLEAR_AND_SET_16     ; 20 !
        WASTE_9                 ; 29 !
        ____SPKR_DUTY____4      ; 33 !
        
        KBD_LOAD_7              ; 40
        WASTE_5                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        ldx     #$00            ; 51 !
        WASTE_12                ; 63 !
s25:    SER_AVAIL_A_6           ; 69 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 71 ! 72
        WASTE_3                 ; 74 !
        ____SPKR_DUTY____4      ; 78 !

d25:    SER_FETCH_DEST_A_4      ; 82      - yes
        sta     dest25+2,x      ; 87
dest25:
        jmp     $0000           ; 90
:
        WASTE_2                 ;    74 !
        ____SPKR_DUTY____4      ;    78 !
        WASTE_9                 ;    87
        jmp     duty_cycle25    ;    90


.align 256
.assert * = _SAMPLES_BASE+$1A00, error
duty_cycle26:
        ____SPKR_DUTY____4      ; 4  !
        VU_CLEAR_AND_SET_16     ; 20 !
        WASTE_10                ; 30 !
        ____SPKR_DUTY____4      ; 34 !
        
        KBD_LOAD_7              ; 41
        WASTE_4                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_15                ; 64 !
s26:    SER_AVAIL_A_6           ; 70 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 72 ! 73
        WASTE_3                 ; 75 !
        ____SPKR_DUTY____4      ; 79 !

d26:    SER_FETCH_DEST_A_4      ; 83      - yes
        sta     dest26+2        ; 87
dest26:
        jmp     $0000           ; 90
:
        WASTE_2                 ;    75 !
        ____SPKR_DUTY____4      ;    79 !
        WASTE_8                 ;    87
        jmp     duty_cycle26    ;    90


.align 256
.assert * = _SAMPLES_BASE+$1B00, error
duty_cycle27:
        ____SPKR_DUTY____4      ; 4  !
        VU_CLEAR_AND_SET_16     ; 20 !
        WASTE_11                ; 31 !
        ____SPKR_DUTY____4      ; 35 !
        
        KBD_LOAD_7              ; 42
        WASTE_3                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_15                ; 64 !
s27:    SER_AVAIL_A_6           ; 70 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 72 ! 73
d27:    SER_FETCH_DEST_A_4      ; 76      - yes
        ____SPKR_DUTY____4      ; 80 !

        WASTE_3                 ; 83
        sta     dest27+2        ; 87
dest27:
        jmp     $0000           ; 90
:
        WASTE_3                 ;    76 !
        ____SPKR_DUTY____4      ;    80 !
        WASTE_7                 ;    87
        jmp     duty_cycle27    ;    90


.align 256
.assert * = _SAMPLES_BASE+$1C00, error
duty_cycle28:
        ____SPKR_DUTY____4      ; 4  !
        VU_CLEAR_AND_SET_16     ; 20 !
        WASTE_12                ; 32 !
        ____SPKR_DUTY____4      ; 36 !
        
        KBD_LOAD_7              ; 43
        WASTE_2                 ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_16                ; 65 !
s28:    SER_AVAIL_A_6           ; 71 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 73 ! 74
d28:    SER_FETCH_DEST_A_4      ; 77 !
        ____SPKR_DUTY____4      ; 81 !

        WASTE_2                 ; 83
        sta     dest28+2        ; 87
dest28:
        jmp     $0000           ; 90
:
        WASTE_3                 ;    77 !
        ____SPKR_DUTY____4      ;    81 !
        WASTE_6                 ;    87
        jmp     duty_cycle28    ;    90


.align 256
.assert * = _SAMPLES_BASE+$1D00, error
duty_cycle29:
        ____SPKR_DUTY____4      ; 4  !
        VU_CLEAR_AND_SET_16     ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_6                 ; 33 !
        ____SPKR_DUTY____4      ; 37 !
        
        WASTE_8                 ; 45
        
        ____SPKR_DUTY____4      ; 49 !
        ldx     #$00            ; 51 !
        WASTE_15                ; 66 !
s29:    SER_AVAIL_A_6           ; 72 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 74 ! 75
d29:    SER_FETCH_DEST_A_4      ; 78 !
        ____SPKR_DUTY____4      ; 82 !

        sta     dest29+2,x      ; 87
dest29:
        jmp     $0000           ; 90
:
        WASTE_3                 ;    78 !
        ____SPKR_DUTY____4      ;    82 !
        WASTE_5                 ;    87
        jmp     duty_cycle29    ;    90


.align 256
.assert * = _SAMPLES_BASE+$1E00, error
duty_cycle30:
        ____SPKR_DUTY____4      ; 4  !
        VU_CLEAR_AND_SET_16     ; 20 !
        WASTE_14                ; 34 !
        ____SPKR_DUTY____4      ; 38 !
        
        KBD_LOAD_7              ; 45

        ____SPKR_DUTY____4      ; 49 !
        WASTE_16                ; 65 !
s30:    SER_AVAIL_A_6           ; 71 !
        WASTE_2                 ; 73 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 75 ! 76
d30:    SER_FETCH_DEST_A_4      ; 79 !
        ____SPKR_DUTY____4      ; 83 !

        sta     dest30+2        ; 87
dest30:
        jmp     $0000           ; 90
:
        WASTE_3                 ;    79 !
        ____SPKR_DUTY____4      ;    83 !
        WASTE_4                 ;    87
        jmp     duty_cycle30    ;    90


.align 256
.assert * = _SAMPLES_BASE+$1F00, error
duty_cycle31:
        ____SPKR_DUTY____4      ; 4  !
        VU_CLEAR_AND_SET_16     ; 20 !
        WASTE_11                ; 31 !
        lda     KBD             ; 35 !
        ____SPKR_DUTY____4      ; 39 !
        WASTE_3                 ; 42
        bpl     :+              ; 45
        jsr     kbd_send

:       ____SPKR_DUTY____4      ; 49 !
        WASTE_15                ; 64 !
s31:    SER_AVAIL_A_6           ; 70 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 72 ! 73
d31:    SER_FETCH_DEST_A_4      ; 76 !
        sta     dest31+2        ; 80 !
        ____SPKR_DUTY____4      ; 84 !
        WASTE_3                 ; 87
dest31:
        jmp     $0000           ; 90
:
        WASTE_7                 ;    80 !
        ____SPKR_DUTY____4      ;    84 !
        WASTE_3                 ;    87
        jmp     duty_cycle31    ;    90

.align 256
.assert * = _SAMPLES_BASE+$2000, error
break_out:
.ifdef IIGS
        lda     prevspd
        jsr     _set_iigs_speed
.endif
        lda     #$01            ; Reenable IRQ and flush
        jsr     _simple_serial_set_irq
        jmp     _simple_serial_flush

silence:
ssil:   lda     status
        and     #HAS_BYTE
        beq     silence
dsil:   lda     data
        sta     start_duty+2
start_duty:
        jmp     $0000

_pwm:
        pha
        ; Disable interrupts
        lda     #$00
        jsr     _simple_serial_set_irq

        ; Setup vumeter pointer
        pla
        sta     CV
        jsr     VTABZ
        jsr     popa
        sta     CH
        tay
        pha
        lda     #'('|$80
        sta     (BASL),y
        tya
        clc
        adc     #33
        sta     CH
        tay
        lda     #')'|$80
        sta     (BASL),y
        pla
        clc
        adc     #1
        adc     BASL
        sta     vumeter_base
        lda     BASH
        adc     #0
        sta     vumeter_base+1

        ; Setup pointer access to SPKR
:       lda     #<(SPKR)
        sta     spkr_ptr
        lda     #>(SPKR)
        sta     spkr_ptr+1

        ; Setup pointer access to dummy var
        lda     #<(dummy_abs)
        sta     dummy_ptr
        lda     #>(dummy_abs)
        sta     dummy_ptr+1

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
        sta     ssil+2

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
        sta     dsil+2

.ifdef IIGS
        ; Slow down IIgs
        jsr     _get_iigs_speed
        sta     prevspd
        lda     #SPEED_SLOW
        jsr     _set_iigs_speed
.endif
        ; Start with silence
        ldy     #15              ; Vumeter
        jmp     silence

kbd_send:
        and     #$7F            ; Clear high bit
        bit     KBDSTRB         ; Clear keyboard strobe
        jmp     _serial_putc_direct

        .bss
dummy_abs: .res 1
counter:   .res 1
prevspd:   .res 1
