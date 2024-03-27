;
; Colin Leroy-Mira, 2024
;
        .export         _pwm
        .export         _SAMPLES_BASE

        .importzp       _zp6, _zp8, _zp10, _zp12, tmp1, tmp2, ptr1, ptr2

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

KBD_OFFSET   := KBD-2           ; To cross page

data         := $FFFF           ; Placeholders for legibility, going to be patched
status       := $FFFF

spkr_ptr      = _zp6
dummy_ptr     = _zp8
dummy_zp      = tmp1
vumeter_base  = ptr1
vumeter_ptr   = ptr2

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
        inc     dummy_abs
.endmacro

.macro KBD_LOAD_7               ; Check keyboard and jsr if key pressed (trashes A)
        lda     KBD             ; 4
        bpl     :+              ; 7
        jsr     kbd_send
:
.endmacro

.macro KBD_LOAD_8               ; Check keyboard and jsr if key pressed (X=2, trashes A)
        lda     KBD_OFFSET,x    ; 5
        bpl     :+              ; 8
        jsr     kbd_send
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

.macro VU_START_CLEAR_3         ; Start clearing previous VU meter level (trashes A)
        lda     tmp2            ; 3
.endmacro

.macro VU_END_CLEAR_5           ; Finish clearing previous VU meter level
        sta     $FFFF,y         ; 5
.endmacro

.macro VU_END_CLEAR_A_6         ; Finish clearing VU meter level (indexed)
        sta     (vumeter_ptr),y ; 6
.endmacro

.macro VU_START_SET_Y_2         ; Start setting new VU level (trashes A)
        ldy     #>(*-_SAMPLES_BASE) ; 2
.endmacro

.macro VU_START_SET_A_2         ; Second part of VU setting start
        lda     #' '            ; 2
.endmacro

.macro VU_START_SET_X_2         ; Second part of VU setting start
        ldx     #' '            ; 2
.endmacro

.macro VU_START_SET_A_4           ; Both parts of new VU meter level set start (trashes A)
        VU_START_SET_Y_2
        VU_START_SET_A_2
.endmacro

.macro VU_START_SET_X_4           ; Both parts of new VU meter level set start (trashes A)
        VU_START_SET_Y_2
        VU_START_SET_X_2
.endmacro

.macro VU_END_SET_X_4           ; Finish new VU meter level set (direct)
        stx     $FFFF           ; 4
.endmacro

.macro VU_END_SET_A_5             ; Finish new VU meter level set (indexed)
        sta     $FFFF,y         ; 5
.endmacro

.macro VU_END_SET_A_6             ; Finish new VU meter level set (indexed)
        sta     (vumeter_ptr),y   ; 6
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
        VU_START_SET_A_4        ; 19
vs0:    VU_END_SET_A_5          ; 24
        WASTE_2                 ; 26

s0:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d0:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest0+2         ; 42
dest0:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
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
        VU_START_CLEAR_2        ; 11
        .else
        ____SPKR_DUTY____4      ; 8  ! Approximation here
        VU_START_CLEAR_3        ; 11
        .endif

vc1:    VU_END_CLEAR_5          ; 16
        VU_START_SET_A_4        ; 20
        VU_END_SET_A_6          ; 26

s1:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d1:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest1+2         ; 42
dest1:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle1     ;    45

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
        sta     vumeter_ptr
        lda     BASH
        adc     #0
        sta     vumeter_base+1
        sta     vumeter_ptr+1

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

        ; Setup a space for vumeter
        lda     #' '|$80
        sta     tmp2
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
        VU_START_SET_A_4        ; 21
vs2:    VU_END_SET_A_5          ; 26

s2:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d2:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest2+2         ; 42
dest2:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle2     ;    45

; ------------------------------------------------------------------
patch_status_register_low:
        ; Patch ALL the serial regs!
        ; Status low byte
        lda     serial_status_reg+1
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
        rts
; ------------------------------------------------------------------

.align 256
.assert * = _SAMPLES_BASE+$300, error
duty_cycle3:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_3        ; 7 !
        ____SPKR_DUTY____4      ; 11 !

vc3:    VU_END_CLEAR_5          ; 16
        VU_START_SET_A_4        ; 20
        VU_END_SET_A_6          ; 26

s3:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d3:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest3+2         ; 42
dest3:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle3     ;    45

; ------------------------------------------------------------------
patch_data_register_low:
        ; Data low byte
        lda     serial_data_reg+1
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
        VU_START_SET_A_4        ; 21
vs4:    VU_END_SET_A_5          ; 26

s4:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d4:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest4+2         ; 42
dest4:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle4     ;    45

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
        jsr     patch_vumeter_addr_direct_a
        jsr     patch_vumeter_addr_direct_b

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
        VU_START_CLEAR_2        ; 6 !
        WASTE_3                 ; 9 !
        ____SPKR_DUTY____4      ; 13 !

vc5:    VU_END_CLEAR_5          ; 18
        VU_START_SET_X_4        ; 22
vsd5:   VU_END_SET_X_4          ; 26

s5:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d5:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest5+2         ; 42
dest5:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle5     ;    45

patch_vumeter_addr_low:
        lda     vumeter_base
        sta     vc0+1
        sta     vc1+1
        sta     vs0+1
        sta     vc2+1
        sta     vs2+1
        sta     vc3+1
        sta     vc4+1
        sta     vs4+1
        sta     vc5+1
        sta     vc7+1
        sta     vs7+1
        sta     vc9+1
        sta     vs9+1
        sta     vc10+1
        sta     vc11+1
        sta     vs11+1
        sta     vc13+1
        sta     vs13+1
        sta     vc15+1
        sta     vc16+1
        sta     vs16+1
        sta     vc17+1
        sta     vc18+1
        sta     vs18+1
        sta     vc19+1
        sta     vc20+1
        sta     vc21+1
        sta     vc22+1
        sta     vs22+1
        sta     vc23+1
        sta     vc24+1
        sta     vs24+1
        sta     vc25+1
        sta     vc26+1
        sta     vc27+1
        sta     vc28+1
        sta     vs28+1
        sta     vc29+1
        sta     vc30+1
        sta     vs30+1
        sta     vc31+1
        sta     vc32+1
        sta     vs32+1
        rts

.align 256
.assert * = _SAMPLES_BASE+$600, error
duty_cycle6:
        ____SPKR_DUTY____4      ; 4 !
s6:     SER_AVAIL_A_6           ; 10 !
        ____SPKR_DUTY____4      ; 14 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 16 17

        VU_START_CLEAR_2        ; 18
        VU_END_CLEAR_A_6        ; 24
        VU_START_SET_A_4        ; 28
        VU_END_SET_A_6          ; 34

d6:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest6+2         ; 42
dest6:
        jmp     $0000           ; 45
:
        VU_START_CLEAR_2        ;    19
        VU_END_CLEAR_A_6        ;    25
        VU_START_SET_A_4        ;    29
        VU_END_SET_A_6          ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle6     ;    45

patch_status_register_high:
        ; Status high byte
        lda     serial_status_reg+2
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

.align 256
.assert * = _SAMPLES_BASE+$700, error
duty_cycle7:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc7:    VU_END_CLEAR_5          ; 11 !
        ____SPKR_DUTY____4      ; 15 !

        VU_START_SET_A_4        ; 19
vs7:    VU_END_SET_A_5          ; 24
        WASTE_2                 ; 26

s7:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d7:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest7+2         ; 42
dest7:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle7     ;    45

patch_data_register_high:
        ; Data high byte
        lda     serial_data_reg+2
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

.align 256
.assert * = _SAMPLES_BASE+$800, error
duty_cycle8:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
        VU_END_CLEAR_A_6        ; 12 !
        ____SPKR_DUTY____4      ; 16 !

        VU_START_SET_A_4        ; 20
        VU_END_SET_A_6          ; 26

s8:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d8:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest8+2         ; 42
dest8:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle8     ;    45

patch_vumeter_addr_high:
        lda     vumeter_base+1
        sta     vc0+2
        sta     vs0+2
        sta     vc1+2
        sta     vc2+2
        sta     vs2+2
        sta     vc3+2
        sta     vc4+2
        sta     vs4+2
        sta     vc5+2
        sta     vc7+2
        sta     vs7+2
        sta     vc9+2
        sta     vs9+2
        sta     vc10+2
        sta     vc11+2
        sta     vs11+2
        sta     vc13+2
        sta     vs13+2
        sta     vc15+2
        sta     vc16+2
        sta     vs16+2
        sta     vc17+2
        sta     vc18+2
        sta     vs18+2
        sta     vc19+2
        sta     vc20+2
        sta     vc21+2
        sta     vc22+2
        sta     vs22+2
        sta     vc23+2
        sta     vc24+2
        sta     vs24+2
        sta     vc25+2
        sta     vc26+2
        sta     vc27+2
        sta     vc28+2
        sta     vs28+2
        sta     vc29+2
        sta     vc30+2
        sta     vs30+2
        sta     vc31+2
        sta     vc32+2
        sta     vs32+2
        rts

.align 256
.assert * = _SAMPLES_BASE+$900, error
duty_cycle9:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc9:    VU_END_CLEAR_5          ; 11 !
        VU_START_SET_Y_2        ; 13 !
        ____SPKR_DUTY____4      ; 17 !

        VU_START_SET_A_2        ; 19
vs9:    VU_END_SET_A_5          ; 24
        WASTE_2                 ; 26

s9:     SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d9:     SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest9+2         ; 42
dest9:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle9     ;    45

patch_vumeter_addr_direct_a:
        lda     vumeter_base+1
        sta     vsd5+2
        sta     vsd10+2
        sta     vsd14+2
        sta     vsd15+2
        sta     vsd17+2
        sta     vsd19+2
        sta     vsd20+2
        sta     vsd21+2
        sta     vsd23+2
        sta     vsd25+2
        sta     vsd26+2
        sta     vsd27+2
        sta     vsd29+2
        sta     vsd31+2
        clc
        lda     vumeter_base
        adc     #5
        sta     vsd5+1
        bcc     :+
        inc     vsd5+2
        clc
:       lda     vumeter_base
        adc     #10
        sta     vsd10+1
        bcc     :+
        inc     vsd10+2
        clc
:       lda     vumeter_base
        adc     #14
        sta     vsd14+1
        bcc     :+
        inc     vsd14+2
        clc
:       lda     vumeter_base
        adc     #15
        sta     vsd15+1
        bcc     :+
        inc     vsd15+2
        clc
:       lda     vumeter_base
        adc     #17
        sta     vsd17+1
        bcc     :+
        inc     vsd17+2
        clc
:       rts

.align 256
.assert * = _SAMPLES_BASE+$A00, error
duty_cycle10:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc10:   VU_END_CLEAR_5          ; 11 !
        WASTE_3                 ; 14 !
        ____SPKR_DUTY____4      ; 18 !

        VU_START_SET_X_4        ; 22
vsd10:  VU_END_SET_X_4          ; 26

s10:    SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d10:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest10+2        ; 42
dest10:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle10    ;    45

patch_vumeter_addr_direct_b:
        clc
        lda     vumeter_base
        adc     #19
        sta     vsd19+1
        bcc     :+
        inc     vsd19+2
        clc
:       lda     vumeter_base
        adc     #20
        sta     vsd20+1
        bcc     :+
        inc     vsd20+2
        clc
:       lda     vumeter_base
        adc     #21
        sta     vsd21+1
        bcc     :+
        inc     vsd21+2
        clc
:       lda     vumeter_base
        adc     #23
        sta     vsd23+1
        bcc     :+
        inc     vsd23+2
        clc
:       lda     vumeter_base
        adc     #25
        sta     vsd25+1
        bcc     :+
        inc     vsd25+2
        clc
:       lda     vumeter_base
        adc     #26
        sta     vsd26+1
        bcc     :+
        inc     vsd26+2
        clc
:       lda     vumeter_base
        adc     #27
        sta     vsd27+1
        bcc     :+
        inc     vsd27+2
        clc
:       lda     vumeter_base
        adc     #29
        sta     vsd29+1
        bcc     :+
        inc     vsd29+2
        clc
:       lda     vumeter_base
        adc     #31
        sta     vsd31+1
        bcc     :+
        inc     vsd31+2
        clc
:       rts

.align 256
.assert * = _SAMPLES_BASE+$B00, error
duty_cycle11:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc11:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_A_4        ; 15 !
        ____SPKR_DUTY____4      ; 19 !

vs11:   VU_END_SET_A_5          ; 24
        WASTE_2                 ; 26

s11:    SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d11:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest11+2        ; 42
dest11:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle11    ;    45


.align 256
.assert * = _SAMPLES_BASE+$C00, error
duty_cycle12:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
        VU_END_CLEAR_A_6        ; 12 !
        VU_START_SET_A_4        ; 16 !
        ____SPKR_DUTY____4      ; 20 !

        VU_END_SET_A_6          ; 26

s12:    SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d12:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest12+2        ; 42
dest12:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle12    ;    45


.align 256
.assert * = _SAMPLES_BASE+$D00, error
duty_cycle13:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc13:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_A_4        ; 15 !
        WASTE_2                 ; 17 !
        ____SPKR_DUTY____4      ; 21 !

vs13:   VU_END_SET_A_5          ; 26

s13:    SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d13:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest13+2        ; 42
dest13:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle13    ;    45

.align 256
.assert * = _SAMPLES_BASE+$E00, error
duty_cycle14:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
        VU_END_CLEAR_A_6        ; 12 !
        VU_START_SET_X_4        ; 16 !
        WASTE_2                 ; 18 !
        ____SPKR_DUTY____4      ; 22 !

vsd14:  VU_END_SET_X_4          ; 26

s14:    SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d14:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest14+2        ; 42
dest14:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle14    ;    45


.align 256
.assert * = _SAMPLES_BASE+$F00, error
duty_cycle15:                   ; Middle level, zero, 23 cycles
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc15:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_X_4        ; 15 !
vsd15:  VU_END_SET_X_4          ; 19 !
        ____SPKR_DUTY____4      ; 23 !

        WASTE_3                 ; 26

s15:    SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d15:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest15+2        ; 42
dest15:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle15    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1000, error
duty_cycle16:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc16:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_A_4        ; 15 !
vs16:   VU_END_SET_A_5          ; 20 !
        ____SPKR_DUTY____4      ; 24 !

        WASTE_2                 ; 26

s16:    SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d16:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest16+2        ; 42
dest16:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle16    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1100, error
duty_cycle17:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc17:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_X_4        ; 15 !
vsd17:  VU_END_SET_X_4          ; 19 !
        ldx     #$02            ; 21 !
        ____SPKR_DUTY____4      ; 25 !

s17:    SER_AVAIL_A_6           ; 31
        SER_LOOP_IF_NOT_AVAIL_2 ; 33 34
d17:    SER_FETCH_DEST_A_4      ; 37      - yes
        sta     dest17,x        ; 42
dest17:
        jmp     $0000           ; 45
:
        KBD_LOAD_8              ;    42 (X=2!)
        jmp     duty_cycle17    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1200, error
duty_cycle18:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc18:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_A_4        ; 15 !
vs18:   VU_END_SET_A_5          ; 20 !
        WASTE_2                 ; 22 !
        ____SPKR_DUTY____4      ; 26 !

s18:    SER_AVAIL_A_6           ; 32
        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d18:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest18+2        ; 42
dest18:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle18    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1300, error
duty_cycle19:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc19:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_X_4        ; 15 !
        WASTE_2                 ; 17 !
s19:    SER_AVAIL_A_6           ; 23 !
        ____SPKR_DUTY____4      ; 27 !

vsd19:  VU_END_SET_X_4          ; 31

        SER_LOOP_IF_NOT_AVAIL_2 ; 33 34
d19:    SER_FETCH_DEST_A_4      ; 37      - yes
        sta     dest19+2-19,y   ; 42      - -19 because Y=19
dest19:
        jmp     $0000           ; 45
:
        lda     KBD-19,y        ;    39
        bpl     :+              ;    42
        jsr     kbd_send
:       jmp     duty_cycle19    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1400, error
duty_cycle20:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc20:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_X_4        ; 15 !
        WASTE_3                 ; 18 !
s20:    SER_AVAIL_A_6           ; 24 !
        ____SPKR_DUTY____4      ; 28 !

vsd20:  VU_END_SET_X_4          ; 32

        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d20:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest20+2        ; 42
dest20:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle20    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1500, error
duty_cycle21:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc21:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_X_4        ; 15 !
vsd21:  VU_END_SET_X_4          ; 19 !
s21:    SER_AVAIL_A_6           ; 25 !
        ____SPKR_DUTY____4      ; 29 !

        WASTE_3                 ; 32

        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d21:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest21+2        ; 42
dest21:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle21    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1600, error
duty_cycle22:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc22:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_A_4        ; 15 !
vs22:   VU_END_SET_A_5          ; 20 !
s22:    SER_AVAIL_A_6           ; 26 !
        ____SPKR_DUTY____4      ; 30 !

        WASTE_2                 ; 32

        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d22:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest22+2        ; 42
dest22:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle22    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1700, error
duty_cycle23:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc23:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_X_4        ; 15 !
vsd23:  VU_END_SET_X_4          ; 19 !
        ldx     #$02            ; 21 !
s23:    SER_AVAIL_A_6           ; 27 !
        ____SPKR_DUTY____4      ; 31 !

        SER_LOOP_IF_NOT_AVAIL_2 ; 33 35
d23:    SER_FETCH_DEST_A_4      ; 37      - yes
        sta     dest23,x        ; 42
dest23:
        jmp     $0000           ; 45
:
        KBD_LOAD_8              ;    42 (X=2!)
        jmp     duty_cycle23    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1800, error
duty_cycle24:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc24:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_A_4        ; 15 !
vs24:   VU_END_SET_A_5          ; 20 !
        WASTE_2                 ; 22 !
s24:    SER_AVAIL_A_6           ; 28 !
        ____SPKR_DUTY____4      ; 32 !

        SER_LOOP_IF_NOT_AVAIL_2 ; 34 35
d24:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest24+2        ; 42
dest24:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    42
        jmp     duty_cycle24    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1900, error
duty_cycle25:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc25:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_X_4        ; 15 !
        WASTE_3                 ; 18 !
s25:    SER_AVAIL_A_6           ; 24 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 26 ! 27
        WASTE_3                 ; 29 !
        ____SPKR_DUTY____4      ; 33 !

d25:    SER_FETCH_DEST_A_4      ; 37      - yes
        sta     dest25-25+2,y   ; 42
dest25:
        jmp     $0000           ; 45
:
        WASTE_2                 ;    29 !
        ____SPKR_DUTY____4      ;    33 !
vsd25:  VU_END_SET_X_4          ;    37
        WASTE_5                 ;    42
        jmp     duty_cycle25    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1A00, error
duty_cycle26:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc26:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_X_4        ; 15 !
vsd26:  VU_END_SET_X_4          ; 19 !
s26:    SER_AVAIL_A_6           ; 25 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 27 ! 28
        WASTE_3                 ; 30 !
        ____SPKR_DUTY____4      ; 34 !

d26:    SER_FETCH_DEST_A_4      ; 38      - yes
        sta     dest26+2        ; 42
dest26:
        jmp     $0000           ; 45
:
        ldx     #$02            ;    30 !
        ____SPKR_DUTY____4      ;    34 !
        KBD_LOAD_8              ;    42 (X=2!)
        jmp     duty_cycle26    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1B00, error
duty_cycle27:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc27:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_X_4        ; 15 !
vsd27:  VU_END_SET_X_4          ; 19 !
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
        KBD_LOAD_7              ;    42
        jmp     duty_cycle27    ;    45


.align 256
.assert * = _SAMPLES_BASE+$1C00, error
duty_cycle28:
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc28:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_A_4        ; 15 !
vs28:   VU_END_SET_A_5          ; 20 !
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
        VU_START_CLEAR_2        ; 8 !
vc29:   VU_END_CLEAR_5          ; 13 !
        VU_START_SET_X_4        ; 15 !
vsd29:  VU_END_SET_X_4          ; 19 !
        ldx     #$02            ; 6 !
s29:    SER_AVAIL_A_6           ; 27 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 29 ! 30
d29:    SER_FETCH_DEST_A_4      ; 33 !
        ____SPKR_DUTY____4      ; 37 !

        sta     dest29,x        ; 42
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
        VU_START_CLEAR_2        ; 6 !
vc30:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_A_4        ; 15 !
vs30:   VU_END_SET_A_5          ; 20 !
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
        VU_START_CLEAR_2        ; 6 !
vc31:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_X_4        ; 15 !
vsd31:  VU_END_SET_X_4          ; 19 !
s31:    SER_AVAIL_A_6           ; 25 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 27 ! 28
d31:    SER_FETCH_DEST_A_4      ; 31 !
        sta     dest31+2        ; 35 !
        ____SPKR_DUTY____4      ; 39 !
        WASTE_3                 ; 42
dest31:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    35 !
        ____SPKR_DUTY____4      ;    39 !
        WASTE_3                 ;    42
        jmp     duty_cycle31    ;    45

.align 256
.assert * = _SAMPLES_BASE+$2000, error
duty_cycle32:                   ; Max positive level, 40 cycles
        ____SPKR_DUTY____4      ; 4 !
        VU_START_CLEAR_2        ; 6 !
vc32:   VU_END_CLEAR_5          ; 11 !
        VU_START_SET_A_4        ; 15 !
vs32:   VU_END_SET_A_5          ; 20 !
s32:    SER_AVAIL_A_6           ; 26 !
        SER_LOOP_IF_NOT_AVAIL_2 ; 28 ! 29
d32:    SER_FETCH_DEST_A_4      ; 32 !
        sta     dest32+2        ; 36 !
        ____SPKR_DUTY____4      ; 40 !
        WASTE_2                 ; 42
dest32:
        jmp     $0000           ; 45
:
        KBD_LOAD_7              ;    36 !
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
        jsr     _simple_serial_flush
        lda     #$2F            ; SURL_CLIENT_READY
        jmp     _serial_putc_direct

        .bss
dummy_abs: .res 1
counter:   .res 1
prevspd:   .res 1
