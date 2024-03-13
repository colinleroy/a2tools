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

MAX_LEVEL         = 32

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
.macro WASTE_20
        WASTE_18
        WASTE_2
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

.macro VU_START_CLEAR_2         ; Start clearing previous VU meter level (trashes A)
        lda     #' '|$80        ; 2
.endmacro

.macro VU_END_CLEAR_5           ; Finish clearing previous VU meter level
        sta     $FFFF,y         ; 5
.endmacro

.macro VU_START_SET_A_2         ; Start setting new VU level (trashes A)
        ldy     #>(*-_SAMPLES_BASE) ; 2
.endmacro

.macro VU_START_SET_B_2         ; Second part of VU setting start
        lda     #' '            ; 2
.endmacro

.macro VU_START_SET_4           ; Both parts of new VU meter level set start (trashes A)
        VU_START_SET_A_2
        VU_START_SET_B_2
.endmacro

.macro VU_START_SET_DIRECT_4    ; Both parts of new VU meter level set start (trashes X)
        VU_START_SET_A_2        ; 2
        ldx     #' '            ; 4
.endmacro

.macro VU_END_SET_DIRECT_4      ; Finish new VU meter level set (direct, from X)
        stx     $FFFF           ; 4
.endmacro

.macro VU_END_SET_5             ; Finish new VU meter level set (indexed)
        sta     $FFFF,y         ; 5
.endmacro

        .code

.align 256
_SAMPLES_BASE = *
.assert * = $4000, error
duty_cycle0:                    ; Max negative level, 8 cycles
        ____SPKR_DUTY____4      ; 4  !
        ____SPKR_DUTY____4      ; 8  !

        VU_START_CLEAR_2        ; 10
vc0:    VU_END_CLEAR_5          ; 15
        VU_START_SET_4          ; 19
vs0:    VU_END_SET_5            ; 24
        WASTE_2                 ; 26
        KBD_LOAD_7              ; 33
        WASTE_13                ; 46

s0:     SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d0:     SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest0+2         ; 62
dest0:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle0     ;    65

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

        VU_START_CLEAR_2        ; 28
vc1:    VU_END_CLEAR_5          ; 33
        VU_START_SET_4          ; 37
vs1:    VU_END_SET_5            ; 42
        WASTE_4                 ; 46

s1:     SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d1:     SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest1+2         ; 62
dest1:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle1     ;    65

; ------------------------------------------------------------------
setup_pointers:
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
        adc     #(MAX_LEVEL+2)
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

        VU_START_CLEAR_2        ; 12
vc2:    VU_END_CLEAR_5          ; 17
        VU_START_SET_4          ; 21
vs2:    VU_END_SET_5            ; 26

        KBD_LOAD_7              ; 33
        WASTE_13                ; 46

s2:     SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d2:     SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest2+2         ; 62
dest2:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle2     ;    65

; ------------------------------------------------------------------
patch_status_register_low:
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
        sta     s33+1
        sta     s34+1
        sta     s35+1
        sta     s36+1
        sta     s37+1
        sta     s38+1
        sta     s39+1
        sta     s40+1
        sta     s41+1
        sta     s42+1
        sta     s43+1
        sta     s44+1
        sta     s45+1
        sta     s46+1
        sta     s47+1
        sta     s48+1
        sta     s49+1
        sta     s50+1
        sta     s51+1
        sta     s52+1
        sta     ssil+1
        rts
; ------------------------------------------------------------------

.align 256
.assert * = _SAMPLES_BASE+$300, error
duty_cycle3:
        ____SPKR_DUTY____4      ; 4 !
        WASTE_3                 ; 7 !
        ____SPKR_DUTY____4      ; 11 !

        VU_START_CLEAR_2        ; 13
vc3:    VU_END_CLEAR_5          ; 18
        VU_START_SET_4          ; 22
vs3:    VU_END_SET_5            ; 27
        KBD_LOAD_7              ; 34
        WASTE_12                ; 46

s3:     SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d3:     SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest3+2         ; 62
dest3:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle3     ;    65

; ------------------------------------------------------------------
patch_data_register_low:
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
        sta     d33+1
        sta     d34+1
        sta     d35+1
        sta     d36+1
        sta     d37+1
        sta     d38+1
        sta     d39+1
        sta     d40+1
        sta     d41+1
        sta     d42+1
        sta     d43+1
        sta     d44+1
        sta     d45+1
        sta     d46+1
        sta     d47+1
        sta     d48+1
        sta     d49+1
        sta     d50+1
        sta     d51+1
        sta     d52+1
        sta     dsil+1
        rts
; ------------------------------------------------------------------

.align 256
.assert * = _SAMPLES_BASE+$400, error
duty_cycle4:
        ____SPKR_DUTY____4      ; 4 !
        WASTE_2                 ; 6 !
        VU_START_CLEAR_2        ; 8 !
        ____SPKR_DUTY____4      ; 12 !

vc4:    VU_END_CLEAR_5          ; 17
        VU_START_SET_4          ; 21
vs4:    VU_END_SET_5            ; 26

        KBD_LOAD_7              ; 33
        WASTE_13                ; 46

s4:     SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d4:     SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest4+2         ; 62
dest4:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle4     ;    65

; ------------------------------------------------------------------
_pwm:
        pha
        ; Disable interrupts
        lda     #$00
        jsr     _simple_serial_set_irq

        pla
        ; Setup pointers
        jsr     setup_pointers
        ; Patch serial registers
        jsr     patch_status_register_low
        jsr     patch_status_register_high
        jsr     patch_data_register_low
        jsr     patch_data_register_high
        ; Patch vumeter address
        jsr     patch_vumeter_addr_low
        jsr     patch_vumeter_addr_high
        jsr     patch_vumeter_addr_low_2
        jsr     patch_vumeter_addr_high_2

.ifdef IIGS
        ; Slow down IIgs
        jsr     _get_iigs_speed
        sta     prevspd
        lda     #SPEED_SLOW
        jsr     _set_iigs_speed
.endif
        ; Start with silence
        jmp     silence
; ------------------------------------------------------------------


.align 256
.assert * = _SAMPLES_BASE+$500, error
duty_cycle5:
        ____SPKR_DUTY____4      ; 4 !
        WASTE_5                 ; 9 !
        ____SPKR_DUTY____4      ; 13 !

        WASTE_6                 ; 19
        KBD_LOAD_7              ; 26

        VU_START_CLEAR_2        ; 28
vc5:    VU_END_CLEAR_5          ; 33
        VU_START_SET_4          ; 37
vs5:    VU_END_SET_5            ; 42
        WASTE_4                 ; 46

s5:     SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d5:     SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest5+2         ; 62
dest5:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle5     ;    65

patch_vumeter_addr_low:
        lda     vumeter_base
        sta     vc0+1
        sta     vs0+1
        sta     vc1+1
        sta     vs1+1
        sta     vc2+1
        sta     vs2+1
        sta     vc3+1
        sta     vs3+1
        sta     vc4+1
        sta     vs4+1
        sta     vc5+1
        sta     vs5+1
        sta     vc6+1
        sta     vs6+1
        sta     vc7+1
        sta     vs7+1
        sta     vc8+1
        sta     vs8+1
        sta     vc9+1
        sta     vs9+1
        sta     vc10+1
        sta     vs10+1
        sta     vc11+1
        sta     vs11+1
        sta     vc12+1
        sta     vs12+1
        sta     vc13+1
        sta     vs13+1
        sta     vc14+1
        sta     vs14+1
        sta     vc15+1
        sta     vs15+1
        sta     vc16+1
        sta     vs16+1
        sta     vc17+1
        sta     vs17+1
        sta     vc18+1
        sta     vs18+1
        sta     vc19+1
        sta     vs19+1
        sta     vc20+1
        sta     vs20+1
        sta     vc21+1
        sta     vs21+1
        sta     vc22+1
        sta     vs22+1
        sta     vc23+1
        sta     vs23+1
        sta     vc24+1
        sta     vs24+1
        sta     vc25+1
        sta     vs25+1
        sta     vc26+1
        sta     vs26+1
        sta     vc27+1
        sta     vs27+1
        sta     vc28+1
        sta     vs28+1
        sta     vc29+1
        sta     vs29+1
        sta     vc30+1
        sta     vs30+1
        sta     vc31+1
        sta     vs31+1
        sta     vc32+1
        sta     vs32+1
        rts

.align 256
.assert * = _SAMPLES_BASE+$600, error
duty_cycle6:
        ____SPKR_DUTY____4      ; 4 !
        WASTE_6                 ; 10 !
        ____SPKR_DUTY____4      ; 14 !

        WASTE_5                 ; 19
        KBD_LOAD_7              ; 26

        VU_START_CLEAR_2        ; 28
vc6:    VU_END_CLEAR_5          ; 33
        VU_START_SET_4          ; 37
vs6:    VU_END_SET_5            ; 42
        WASTE_4                 ; 46

s6:     SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d6:     SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest6+2         ; 62
dest6:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle6     ;    65

patch_status_register_high:
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
        sta     s33+2
        sta     s34+2
        sta     s35+2
        sta     s36+2
        sta     s37+2
        sta     s38+2
        sta     s39+2
        sta     s40+2
        sta     s41+2
        sta     s42+2
        sta     s43+2
        sta     s44+2
        sta     s45+2
        sta     s46+2
        sta     s47+2
        sta     s48+2
        sta     s49+2
        sta     s50+2
        sta     s51+2
        sta     s52+2
        sta     ssil+2
        rts

.align 256
.assert * = _SAMPLES_BASE+$700, error
duty_cycle7:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc7:    VU_END_CLEAR_5          ; 11 !
        ____SPKR_DUTY____4      ; 15 !

        VU_START_SET_4          ; 19
vs7:    VU_END_SET_5            ; 24
        WASTE_2                 ; 26

        KBD_LOAD_7              ; 33
        WASTE_13                ; 46

s7:     SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d7:     SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest7+2         ; 62
dest7:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle7     ;    65

patch_data_register_high:
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
        sta     d33+2
        sta     d34+2
        sta     d35+2
        sta     d36+2
        sta     d37+2
        sta     d38+2
        sta     d39+2
        sta     d40+2
        sta     d41+2
        sta     d42+2
        sta     d43+2
        sta     d44+2
        sta     d45+2
        sta     d46+2
        sta     d47+2
        sta     d48+2
        sta     d49+2
        sta     d50+2
        sta     d51+2
        sta     d52+2
        sta     dsil+2
        rts

.align 256
.assert * = _SAMPLES_BASE+$800, error
duty_cycle8:
        ____SPKR_DUTY____4      ; 4 !
        WASTE_8                 ; 12 !
        ____SPKR_DUTY____4      ; 16 !

        KBD_LOAD_7              ; 21

        VU_START_CLEAR_2        ; 23
vc8:    VU_END_CLEAR_5          ; 28
        VU_START_SET_4          ; 32
vs8:    VU_END_SET_5            ; 37
        WASTE_7                 ; 46

s8:     SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d8:     SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest8+2         ; 62
dest8:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle8     ;    65

patch_vumeter_addr_high:
        lda     vumeter_base+1
        sta     vc0+2
        sta     vs0+2
        sta     vc1+2
        sta     vs1+2
        sta     vc2+2
        sta     vs2+2
        sta     vc3+2
        sta     vs3+2
        sta     vc4+2
        sta     vs4+2
        sta     vc5+2
        sta     vs5+2
        sta     vc6+2
        sta     vs6+2
        sta     vc7+2
        sta     vs7+2
        sta     vc8+2
        sta     vs8+2
        sta     vc9+2
        sta     vs9+2
        sta     vc10+2
        sta     vs10+2
        sta     vc11+2
        sta     vs11+2
        sta     vc12+2
        sta     vs12+2
        sta     vc13+2
        sta     vs13+2
        sta     vc14+2
        sta     vs14+2
        sta     vc15+2
        sta     vs15+2
        sta     vc16+2
        sta     vs16+2
        sta     vc17+2
        sta     vs17+2
        sta     vc18+2
        sta     vs18+2
        sta     vc19+2
        sta     vs19+2
        sta     vc20+2
        sta     vs20+2
        sta     vc21+2
        sta     vs21+2
        sta     vc22+2
        sta     vs22+2
        sta     vc23+2
        sta     vs23+2
        sta     vc24+2
        sta     vs24+2
        sta     vc25+2
        sta     vs25+2
        sta     vc26+2
        sta     vs26+2
        sta     vc27+2
        sta     vs27+2
        sta     vc28+2
        sta     vs28+2
        sta     vc29+2
        sta     vs29+2
        sta     vc30+2
        sta     vs30+2
        sta     vc31+2
        sta     vs31+2
        sta     vc32+2
        sta     vs32+2
        rts

.align 256
.assert * = _SAMPLES_BASE+$900, error
duty_cycle9:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc9:    VU_END_CLEAR_5          ; 11 !
        VU_START_SET_A_2        ; 13 !
        ____SPKR_DUTY____4      ; 17 !

        VU_START_SET_B_2        ; 19
vs9:    VU_END_SET_5            ; 24
        WASTE_2                 ; 26

        KBD_LOAD_7              ; 33
        WASTE_13                ; 46

s9:     SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d9:     SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest9+2         ; 62
dest9:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle9     ;    65

patch_vumeter_addr_low_2:
        lda     vumeter_base
        sta     vc33+1
        sta     vs33+1
        sta     vc34+1
        sta     vs34+1
        sta     vc35+1
        sta     vs35+1
        sta     vc36+1
        sta     vs36+1
        sta     vc37+1
        sta     vs37+1
        sta     vc38+1
        sta     vs38+1
        sta     vc39+1
        sta     vs39+1
        sta     vc40+1
        sta     vs40+1
        sta     vc41+1
        sta     vs41+1
        sta     vc42+1
        sta     vs42+1
        sta     vc43+1
        sta     vs43+1
        sta     vc44+1
        sta     vs44+1
        sta     vc45+1
        sta     vs45+1
        sta     vc46+1
        sta     vs46+1
        sta     vc47+1
        sta     vs47+1
        sta     vc48+1
        sta     vs48+1
        sta     vc49+1
        sta     vs49+1
        sta     vc50+1
        sta     vs50+1
        sta     vc51+1
        sta     vs51+1
        sta     vc52+1
        sta     vs52+1
        rts

.align 256
.assert * = _SAMPLES_BASE+$A00, error
duty_cycle10:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_3                 ; 14 !
        ____SPKR_DUTY____4      ; 18 !

        WASTE_8                 ; 26
        VU_START_CLEAR_2        ; 28
vc10:   VU_END_CLEAR_5          ; 33
        VU_START_SET_4          ; 37
vs10:   VU_END_SET_5            ; 42
        WASTE_4                 ; 46

s10:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d10:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest10+2        ; 62
dest10:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle10    ;    65


patch_vumeter_addr_high_2:
        lda     vumeter_base+1
        sta     vc33+2
        sta     vs33+2
        sta     vc34+2
        sta     vs34+2
        sta     vc35+2
        sta     vs35+2
        sta     vc36+2
        sta     vs36+2
        sta     vc37+2
        sta     vs37+2
        sta     vc38+2
        sta     vs38+2
        sta     vc39+2
        sta     vs39+2
        sta     vc40+2
        sta     vs40+2
        sta     vc41+2
        sta     vs41+2
        sta     vc42+2
        sta     vs42+2
        sta     vc43+2
        sta     vs43+2
        sta     vc44+2
        sta     vs44+2
        sta     vc45+2
        sta     vs45+2
        sta     vc46+2
        sta     vs46+2
        sta     vc47+2
        sta     vs47+2
        sta     vc48+2
        sta     vs48+2
        sta     vc49+2
        sta     vs49+2
        sta     vc50+2
        sta     vs50+2
        sta     vc51+2
        sta     vs51+2
        sta     vc52+2
        sta     vs52+2
        rts



.align 256
.assert * = _SAMPLES_BASE+$B00, error
duty_cycle11:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc11:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
        ____SPKR_DUTY____4      ; 19 !

vs11:   VU_END_SET_5            ; 24
        WASTE_2                 ; 26
        KBD_LOAD_7              ; 33
        WASTE_13                ; 46

s11:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d11:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest11+2        ; 62
dest11:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle11    ;    65


.align 256
.assert * = _SAMPLES_BASE+$C00, error
duty_cycle12:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_5                 ; 16 !
        ____SPKR_DUTY____4      ; 20 !

        WASTE_6                 ; 26
        VU_START_CLEAR_2        ; 28
vc12:   VU_END_CLEAR_5          ; 33
        VU_START_SET_4          ; 37
vs12:   VU_END_SET_5            ; 42
        WASTE_4                 ; 46

s12:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d12:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest12+2        ; 62
dest12:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle12    ;    65


.align 256
.assert * = _SAMPLES_BASE+$D00, error
duty_cycle13:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc13:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
        WASTE_2                 ; 17 !
        ____SPKR_DUTY____4      ; 21 !

vs13:   VU_END_SET_5            ; 26

        KBD_LOAD_7              ; 33
        WASTE_13                ; 46

s13:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d13:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest13+2        ; 62
dest13:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle13    ;    65

.align 256
.assert * = _SAMPLES_BASE+$E00, error
duty_cycle14:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_7                 ; 18 !
        ____SPKR_DUTY____4      ; 22 !

        WASTE_4                 ; 26
        VU_START_CLEAR_2        ; 28
vc14:   VU_END_CLEAR_5          ; 33
        VU_START_SET_4          ; 37
vs14:   VU_END_SET_5            ; 42
        WASTE_4                 ; 46

s14:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d14:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest14+2        ; 62
dest14:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle14    ;    65


.align 256
.assert * = _SAMPLES_BASE+$F00, error
duty_cycle15:                   ; Middle level, zero, 23 cycles
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_8                 ; 19 !
        ____SPKR_DUTY____4      ; 23 !

        WASTE_3                 ; 26
        VU_START_CLEAR_2        ; 28
vc15:   VU_END_CLEAR_5          ; 33
        VU_START_SET_4          ; 37
vs15:   VU_END_SET_5            ; 42
        WASTE_4                 ; 46

s15:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d15:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest15+2        ; 62
dest15:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    52
        jmp     duty_cycle15    ;    55


.align 256
.assert * = _SAMPLES_BASE+$1000, error
duty_cycle16:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc16:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs16:   VU_END_SET_5            ; 20 !
        ____SPKR_DUTY____4      ; 24 !

        WASTE_2                 ; 26
        KBD_LOAD_7              ; 33
        WASTE_13                ; 46

s16:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d16:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest16+2        ; 62
dest16:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle16    ;    65


.align 256
.assert * = _SAMPLES_BASE+$1100, error
duty_cycle17:
        ____SPKR_DUTY____4      ; 4 !
        KBD_LOAD_7              ; 11 !
        WASTE_8                 ; 19 !
        ldx     #$00            ; 21 !
        ____SPKR_DUTY____4      ; 25 !

        VU_START_CLEAR_2        ; 27
vc17:   VU_END_CLEAR_5          ; 32
        VU_START_SET_4          ; 36
vs17:   VU_END_SET_5            ; 41
        WASTE_4                 ; 45

s17:    SER_AVAIL_A_6           ; 51
        SER_LOOP_IF_NOT_AVAIL_2 ; 53 54
d17:    SER_FETCH_DEST_A_4      ; 57      - yes
        sta     dest17+2,x      ; 62
dest17:
        jmp     $0000           ; 65
:
        WASTE_8                 ;    62
        jmp     duty_cycle17    ;    65


.align 256
.assert * = _SAMPLES_BASE+$1200, error
duty_cycle18:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc18:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs18:   VU_END_SET_5            ; 20 !
        WASTE_2                 ; 22 !
        ____SPKR_DUTY____4      ; 26 !

        KBD_LOAD_7              ; 33
        WASTE_13                ; 46

s18:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d18:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest18+2        ; 62
dest18:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle18    ;    65


.align 256
.assert * = _SAMPLES_BASE+$1300, error
duty_cycle19:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc19:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs19:   VU_END_SET_5            ; 20 !
        WASTE_3                 ; 23 !
        ____SPKR_DUTY____4      ; 27 !

        KBD_LOAD_7              ; 34
        WASTE_12                ; 46

s19:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d19:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest19+2        ; 62
dest19:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle19    ;    65


.align 256
.assert * = _SAMPLES_BASE+$1400, error
duty_cycle20:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc20:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs20:   VU_END_SET_5            ; 20 !
        WASTE_4                 ; 24 !
        ____SPKR_DUTY____4      ; 28 !

        KBD_LOAD_7              ; 35
        WASTE_11                ; 46

s20:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d20:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest20+2        ; 62
dest20:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle20    ;    65


.align 256
.assert * = _SAMPLES_BASE+$1500, error
duty_cycle21:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc21:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs21:   VU_END_SET_5            ; 20 !
        WASTE_5                 ; 25 !
        ____SPKR_DUTY____4      ; 29 !

        KBD_LOAD_7              ; 36
        WASTE_10                ; 46

s21:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d21:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest21+2        ; 62
dest21:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle21    ;    65


.align 256
.assert * = _SAMPLES_BASE+$1600, error
duty_cycle22:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc22:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs22:   VU_END_SET_5            ; 20 !
        WASTE_6                 ; 26 !
        ____SPKR_DUTY____4      ; 30 !

        KBD_LOAD_7              ; 37
        WASTE_9                 ; 46

s22:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d22:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest22+2        ; 62
dest22:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle22    ;    65


.align 256
.assert * = _SAMPLES_BASE+$1700, error
duty_cycle23:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc23:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs23:   VU_END_SET_5            ; 20 !
        WASTE_7                 ; 27 !
        ____SPKR_DUTY____4      ; 31 !

        KBD_LOAD_7              ; 38
        WASTE_8                 ; 46

s23:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d23:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest23+2        ; 62
dest23:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle23    ;    65


.align 256
.assert * = _SAMPLES_BASE+$1800, error
duty_cycle24:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc24:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs24:   VU_END_SET_5            ; 20 !
        WASTE_8                 ; 28 !
        ____SPKR_DUTY____4      ; 32 !

        KBD_LOAD_7              ; 39
        WASTE_7                 ; 46

s24:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d24:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest24+2        ; 62
dest24:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle24    ;    65


.align 256
.assert * = _SAMPLES_BASE+$1900, error
duty_cycle25:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc25:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs25:   VU_END_SET_5            ; 20 !
        WASTE_9                 ; 29 !
        ____SPKR_DUTY____4      ; 33 !

        KBD_LOAD_7              ; 40
        WASTE_6                 ; 46

s25:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d25:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest25+2        ; 62
dest25:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle25    ;    65


.align 256
.assert * = _SAMPLES_BASE+$1A00, error
duty_cycle26:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc26:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs26:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_3                 ; 30 !
        ____SPKR_DUTY____4      ; 34 !

        WASTE_12                ; 46

s26:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d26:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest26+2        ; 62
dest26:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle26    ;    65


.align 256
.assert * = _SAMPLES_BASE+$1B00, error
duty_cycle27:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc27:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs27:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_4                 ; 31 !
        ____SPKR_DUTY____4      ; 35 !

        WASTE_11                ; 46

s27:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d27:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest27+2        ; 62
dest27:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle27    ;    65


.align 256
.assert * = _SAMPLES_BASE+$1C00, error
duty_cycle28:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc28:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs28:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_5                 ; 32 !
        ____SPKR_DUTY____4      ; 36 !

        WASTE_10                ; 46

s28:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d28:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest28+2        ; 62
dest28:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle28    ;    65


.align 256
.assert * = _SAMPLES_BASE+$1D00, error
duty_cycle29:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc29:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs29:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_6                 ; 33 !
        ____SPKR_DUTY____4      ; 37 !

        WASTE_9                 ; 46

s29:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d29:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest29+2        ; 62
dest29:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle29    ;    65


.align 256
.assert * = _SAMPLES_BASE+$1E00, error
duty_cycle30:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc30:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs30:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_7                 ; 34 !
        ____SPKR_DUTY____4      ; 38 !

        WASTE_8                 ; 46

s30:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d30:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest30+2        ; 62
dest30:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle30    ;    65


.align 256
.assert * = _SAMPLES_BASE+$1F00, error
duty_cycle31:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc31:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs31:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_8                 ; 35 !
        ____SPKR_DUTY____4      ; 39 !

        WASTE_7                 ; 46

s31:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d31:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest31+2        ; 62
dest31:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle31    ;    65

.align 256
.assert * = _SAMPLES_BASE+$2000, error
duty_cycle32:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc32:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs32:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_9                 ; 36 !
        ____SPKR_DUTY____4      ; 40 !

        WASTE_6                 ; 46

s32:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d32:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest32+2        ; 62
dest32:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle32    ;    65


.align 256
.assert * = _SAMPLES_BASE+$2100, error
duty_cycle33:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc33:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs33:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_10                ; 37 !
        ____SPKR_DUTY____4      ; 41 !

        WASTE_5                 ; 46

s33:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d33:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest33+2        ; 62
dest33:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle33    ;    65


.align 256
.assert * = _SAMPLES_BASE+$2200, error
duty_cycle34:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc34:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs34:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_11                ; 38 !
        ____SPKR_DUTY____4      ; 42 !

        WASTE_4                 ; 46

s34:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d34:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest34+2        ; 62
dest34:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle34    ;    65


.align 256
.assert * = _SAMPLES_BASE+$2300, error
duty_cycle35:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc35:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs35:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_12                ; 39 !
        ____SPKR_DUTY____4      ; 43 !

        WASTE_3                 ; 46

s35:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d35:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest35+2        ; 62
dest35:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle35    ;    65


.align 256
.assert * = _SAMPLES_BASE+$2400, error
duty_cycle36:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc36:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs36:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_13                ; 40 !
        ____SPKR_DUTY____4      ; 44 !

        WASTE_2                 ; 46

s36:    SER_AVAIL_A_6           ; 52
        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d36:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest36+2        ; 62
dest36:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle36    ;    65


.align 256
.assert * = _SAMPLES_BASE+$2500, error
duty_cycle37:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc37:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs37:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_7                 ; 34 !
s37:    SER_AVAIL_A_6           ; 40 !
        ____SPKR_DUTY____4      ; 44 !

        WASTE_8                 ; 52

        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d37:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest37+2        ; 62
dest37:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle37    ;    65


.align 256
.assert * = _SAMPLES_BASE+$2600, error
duty_cycle38:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc38:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs38:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_8                 ; 35 !
s38:    SER_AVAIL_A_6           ; 41 !
        ____SPKR_DUTY____4      ; 45 !

        WASTE_7                 ; 52

        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d38:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest38+2        ; 62
dest38:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle38    ;    65


.align 256
.assert * = _SAMPLES_BASE+$2700, error
duty_cycle39:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc39:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs39:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_9                 ; 36 !
s39:    SER_AVAIL_A_6           ; 42 !
        ____SPKR_DUTY____4      ; 46 !

        WASTE_6                 ; 52

        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d39:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest39+2        ; 62
dest39:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle39    ;    65


.align 256
.assert * = _SAMPLES_BASE+$2800, error
duty_cycle40:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc40:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs40:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_10                ; 37 !
s40:    SER_AVAIL_A_6           ; 43 !
        ____SPKR_DUTY____4      ; 47 !

        WASTE_5                 ; 52

        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d40:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest40+2        ; 62
dest40:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle40    ;    65


.align 256
.assert * = _SAMPLES_BASE+$2900, error
duty_cycle41:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc41:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs41:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_11                ; 38 !
s41:    SER_AVAIL_A_6           ; 44 !
        ____SPKR_DUTY____4      ; 48 !

        WASTE_4                 ; 52

        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d41:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest41+2        ; 62
dest41:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle41    ;    65


.align 256
.assert * = _SAMPLES_BASE+$2A00, error
duty_cycle42:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc42:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs42:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_12                ; 39 !
s42:    SER_AVAIL_A_6           ; 45 !
        ____SPKR_DUTY____4      ; 49 !

        WASTE_3                 ; 52

        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d42:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest42+2        ; 62
dest42:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle42    ;    65


.align 256
.assert * = _SAMPLES_BASE+$2B00, error
duty_cycle43:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc43:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs43:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_13                ; 40 !
s43:    SER_AVAIL_A_6           ; 46 !
        ____SPKR_DUTY____4      ; 50 !

        WASTE_2                 ; 52

        SER_LOOP_IF_NOT_AVAIL_2 ; 54 55
d43:    SER_FETCH_DEST_A_4      ; 58      - yes
        sta     dest43+2        ; 62
dest43:
        jmp     $0000           ; 65
:
        WASTE_7                 ;    62
        jmp     duty_cycle43    ;    65


.align 256
.assert * = _SAMPLES_BASE+$2C00, error
duty_cycle44:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc44:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs44:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_4                 ; 31 !
s44:    SER_AVAIL_A_6           ; 37 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 39 ! 40
d44:    SER_FETCH_DEST_A_4      ; 43 !    - yes
        sta     dest44+2        ; 47 !
        ____SPKR_DUTY____4      ; 51 !

        WASTE_11                 ; 62

dest44:
        jmp     $0000           ; 65
:
        WASTE_7                 ; 47 !
        ____SPKR_DUTY____4      ; 51 !
        WASTE_11                ; 62
        jmp     duty_cycle44    ;    65


.align 256
.assert * = _SAMPLES_BASE+$2D00, error
duty_cycle45:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc45:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs45:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_5                 ; 32 !
s45:    SER_AVAIL_A_6           ; 38 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 40 ! 41
d45:    SER_FETCH_DEST_A_4      ; 42 !    - yes
        sta     dest45+2        ; 48 !
        ____SPKR_DUTY____4      ; 52 !

        WASTE_10                 ; 62

dest45:
        jmp     $0000           ; 65
:
        WASTE_7                 ; 48 !
        ____SPKR_DUTY____4      ; 52 !
        WASTE_10                ; 62
        jmp     duty_cycle45    ;    65


.align 256
.assert * = _SAMPLES_BASE+$2E00, error
duty_cycle46:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc46:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs46:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_6                 ; 33 !
s46:    SER_AVAIL_A_6           ; 39 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 41 ! 42
d46:    SER_FETCH_DEST_A_4      ; 43 !    - yes
        sta     dest46+2        ; 49 !
        ____SPKR_DUTY____4      ; 53 !

        WASTE_9                  ; 62

dest46:
        jmp     $0000           ; 65
:
        WASTE_7                 ; 49 !
        ____SPKR_DUTY____4      ; 53 !
        WASTE_9                 ; 62
        jmp     duty_cycle46    ;    65


.align 256
.assert * = _SAMPLES_BASE+$2F00, error
duty_cycle47:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc47:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs47:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_7                 ; 34 !
s47:    SER_AVAIL_A_6           ; 40 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 42 ! 43
d47:    SER_FETCH_DEST_A_4      ; 44 !    - yes
        sta     dest47+2        ; 50 !
        ____SPKR_DUTY____4      ; 54 !

        WASTE_8                 ; 62

dest47:
        jmp     $0000           ; 65
:
        WASTE_7                 ; 50 !
        ____SPKR_DUTY____4      ; 54 !
        WASTE_8                 ; 62
        jmp     duty_cycle47    ;    65


.align 256
.assert * = _SAMPLES_BASE+$3000, error
duty_cycle48:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc48:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs48:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_8                 ; 35 !
s48:    SER_AVAIL_A_6           ; 41 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 43 ! 44
d48:    SER_FETCH_DEST_A_4      ; 45 !    - yes
        sta     dest48+2        ; 51 !
        ____SPKR_DUTY____4      ; 55 !

        WASTE_7                 ; 62

dest48:
        jmp     $0000           ; 65
:
        WASTE_7                 ; 51 !
        ____SPKR_DUTY____4      ; 55 !
        WASTE_7                 ; 62
        jmp     duty_cycle48    ;    65


.align 256
.assert * = _SAMPLES_BASE+$3100, error
duty_cycle49:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc49:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs49:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_9                 ; 36 !
s49:    SER_AVAIL_A_6           ; 42 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 44 ! 45
d49:    SER_FETCH_DEST_A_4      ; 46 !    - yes
        sta     dest49+2        ; 52 !
        ____SPKR_DUTY____4      ; 56 !

        WASTE_6                 ; 62

dest49:
        jmp     $0000           ; 65
:
        WASTE_7                 ; 52 !
        ____SPKR_DUTY____4      ; 56 !
        WASTE_6                 ; 62
        jmp     duty_cycle49    ;    65


.align 256
.assert * = _SAMPLES_BASE+$3200, error
duty_cycle50:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc50:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs50:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_10                ; 37 !
s50:    SER_AVAIL_A_6           ; 43 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 44 ! 46
d50:    SER_FETCH_DEST_A_4      ; 47 !    - yes
        sta     dest50+2        ; 53 !
        ____SPKR_DUTY____4      ; 57 !

        WASTE_5                 ; 62

dest50:
        jmp     $0000           ; 65
:
        WASTE_7                 ; 53 !
        ____SPKR_DUTY____4      ; 57 !
        WASTE_5                 ; 62
        jmp     duty_cycle50    ;    65


.align 256
.assert * = _SAMPLES_BASE+$3300, error
duty_cycle51:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc51:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs51:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_11                ; 38 !
s51:    SER_AVAIL_A_6           ; 44 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 46 ! 47
d51:    SER_FETCH_DEST_A_4      ; 48 !    - yes
        sta     dest51+2        ; 54 !
        ____SPKR_DUTY____4      ; 58 !

        WASTE_4                 ; 62

dest51:
        jmp     $0000           ; 65
:
        WASTE_7                 ; 54 !
        ____SPKR_DUTY____4      ; 58 !
        WASTE_4                 ; 62
        jmp     duty_cycle51    ;    65


.align 256
.assert * = _SAMPLES_BASE+$3400, error
duty_cycle52:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc52:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_4          ; 15 !
vs52:   VU_END_SET_5            ; 20 !
        KBD_LOAD_7              ; 27 !
        WASTE_12                ; 39 !
s52:    SER_AVAIL_A_6           ; 45 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 47 ! 48
d52:    SER_FETCH_DEST_A_4      ; 49 !    - yes
        sta     dest52+2        ; 55 !
        ____SPKR_DUTY____4      ; 59 !

        WASTE_3                 ; 62

dest52:
        jmp     $0000           ; 65
:
        WASTE_7                 ; 55 !
        ____SPKR_DUTY____4      ; 59 !
        WASTE_3                 ; 62
        jmp     duty_cycle52    ;    65


.align 256
.assert * = _SAMPLES_BASE+$3500, error
break_out:
.ifdef IIGS
        lda     prevspd
        jsr     _set_iigs_speed
.endif
        lda     #$01            ; Reenable IRQ and flush
        jsr     _simple_serial_set_irq
        jsr     _simple_serial_flush
        lda     #$2F            ; SURL_CLIENT_READY
        jmp     _serial_putc_direct

        .bss
dummy_abs: .res 1
counter:   .res 1
prevspd:   .res 1
