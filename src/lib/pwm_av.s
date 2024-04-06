;
; Colin Leroy-Mira, 2024
;
        .export         _pwm
        .export         _SAMPLES_BASE

        .importzp       _zp6, _zp8, _zp9, _zp10, _zp12, tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4

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

.macro WASTE_42
        WASTE_12
        WASTE_12
        WASTE_12
        WASTE_6
.endmacro

.macro WASTE_44
        WASTE_12
        WASTE_12
        WASTE_12
        WASTE_8
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
  .macro ____SPKR_DUTY____5       ; Toggle speaker slower (but without phantom-read)
          sta     (spkr_ptr)      ; 5
  .endmacro
.endif

        .code

.align 256
_SAMPLES_BASE = *
.assert * = $6000, error
duty_cycle0:
        ____SPKR_DUTY____4      ; 4
        ____SPKR_DUTY____4      ; 8
as0:    lda     $C0A9           ; 12
        and     #HAS_BYTE       ; 14
        beq     :+              ; 16/17
ad0:    ldx     $C0A8           ; 20
        stx     next+1          ; 23

        WASTE_6                 ; 29
vs0:    lda     $C099           ; 33
        and     #HAS_BYTE       ; 35
        beq     no_vid0         ; 37/38
vd0:    lda     $C098           ; 41
        jmp     video_direct    ; 86 (takes 45 cycles, jumps to next)

:       WASTE_12                ; 29
        jmp     video           ; 88 (takes 57 cycles, jumps to next)

no_vid0:
        WASTE_42                ; 80
        jmp     (next)          ; 86

.align 256
.assert * = _SAMPLES_BASE + $100, error
duty_cycle1:
        ____SPKR_DUTY____4      ; 4
        ____SPKR_DUTY____5      ; 9
        lda     (audio_status)  ; 14
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19
        lda     (audio_data)    ; 23
        sta     next+1          ; 26

        WASTE_15                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_22                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $200, error
duty_cycle2:
        ____SPKR_DUTY____4      ; 4
        WASTE_2
        ____SPKR_DUTY____4      ; 10
        lda     (audio_status)  ; 15
        and     #HAS_BYTE       ; 17
        beq     :+              ; 19/20
        lda     (audio_data)    ; 24
        sta     next+1          ; 27

        WASTE_14                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_21                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $300, error
duty_cycle3:
        ____SPKR_DUTY____4      ; 4
        WASTE_3
        ____SPKR_DUTY____4      ; 11
        lda     (audio_status)  ; 16
        and     #HAS_BYTE       ; 18
        beq     :+              ; 20/21
        lda     (audio_data)    ; 25
        sta     next+1          ; 28

        WASTE_13                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_20                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $400, error
duty_cycle4:
        ____SPKR_DUTY____4      ; 4
        WASTE_4
        ____SPKR_DUTY____4      ; 12
        lda     (audio_status)  ; 17
        and     #HAS_BYTE       ; 19
        beq     :+              ; 21/22
        lda     (audio_data)    ; 26
        sta     next+1          ; 29

        WASTE_12                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_19                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $500, error
duty_cycle5:
        ____SPKR_DUTY____4      ; 4
        WASTE_5
        ____SPKR_DUTY____4      ; 13
        lda     (audio_status)  ; 18
        and     #HAS_BYTE       ; 20
        beq     :+              ; 22/23
        lda     (audio_data)    ; 27
        sta     next+1          ; 30

        WASTE_11                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_18                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $600, error
duty_cycle6:
        ____SPKR_DUTY____4      ; 4
        WASTE_6
        ____SPKR_DUTY____4      ; 14
        lda     (audio_status)  ; 19
        and     #HAS_BYTE       ; 21
        beq     :+              ; 23/24
        lda     (audio_data)    ; 28
        sta     next+1          ; 31

        WASTE_10                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_17                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $700, error
duty_cycle7:
        ____SPKR_DUTY____4      ; 4
        WASTE_7
        ____SPKR_DUTY____4      ; 15
        lda     (audio_status)  ; 20
        and     #HAS_BYTE       ; 22
        beq     :+              ; 24/25
        lda     (audio_data)    ; 29
        sta     next+1          ; 32

        WASTE_9                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_16                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $800, error
duty_cycle8:
        ____SPKR_DUTY____4      ; 4
        WASTE_8
        ____SPKR_DUTY____4      ; 16
        lda     (audio_status)  ; 21
        and     #HAS_BYTE       ; 23
        beq     :+              ; 25/26
        lda     (audio_data)    ; 30
        sta     next+1          ; 33

        WASTE_8                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_15                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $900, error
duty_cycle9:
        ____SPKR_DUTY____4      ; 4
        WASTE_9
        ____SPKR_DUTY____4      ; 17
        lda     (audio_status)  ; 22
        and     #HAS_BYTE       ; 24
        beq     :+              ; 26/27
        lda     (audio_data)    ; 31
        sta     next+1          ; 34

        WASTE_7                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_14                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $A00, error
duty_cycle10:
        ____SPKR_DUTY____4      ; 4
        WASTE_10
        ____SPKR_DUTY____4      ; 18
        lda     (audio_status)  ; 23
        and     #HAS_BYTE       ; 25
        beq     :+              ; 27/28
        lda     (audio_data)    ; 32
        sta     next+1          ; 35

        WASTE_6                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_13                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $B00, error
duty_cycle11:
        ____SPKR_DUTY____4      ; 4
        WASTE_11
        ____SPKR_DUTY____4      ; 19
        lda     (audio_status)  ;
        and     #HAS_BYTE       ;
        beq     :+              ;
        lda     (audio_data)    ;
        sta     next+1          ; 36

        WASTE_5                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_12                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $C00, error
duty_cycle12:
        ____SPKR_DUTY____4      ; 4
        WASTE_12
        ____SPKR_DUTY____4      ; 20
        lda     (audio_status)  ;
        and     #HAS_BYTE       ;
        beq     :+              ;
        lda     (audio_data)    ;
        sta     next+1          ; 37

        WASTE_4                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_11                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $D00, error
duty_cycle13:
        ____SPKR_DUTY____4      ; 4
        WASTE_13
        ____SPKR_DUTY____4      ; 21
        lda     (audio_status)  ;
        and     #HAS_BYTE       ;
        beq     :+              ;
        lda     (audio_data)    ;
        sta     next+1          ; 38

        WASTE_3                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_10                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $E00, error
duty_cycle14:
        ____SPKR_DUTY____4      ; 4
        WASTE_14
        ____SPKR_DUTY____4      ; 22
        lda     (audio_status)  ;
        and     #HAS_BYTE       ;
        beq     :+              ;
        lda     (audio_data)    ;
        sta     next+1          ; 39

        WASTE_2                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_9                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $F00, error
duty_cycle15:
        ____SPKR_DUTY____4      ; 4
        lda     (audio_status)  ; 9
        and     #HAS_BYTE       ; 11
        WASTE_7                 ; 18
        ____SPKR_DUTY____4      ; 22
        beq     :+              ; 24/25
        lda     (audio_data)    ; 29
        sta     next+1          ; 32

        WASTE_9                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_16                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $1000, error
duty_cycle16:
        ____SPKR_DUTY____4      ; 4
        lda     (audio_status)  ; 9
        and     #HAS_BYTE       ; 11
        WASTE_8                 ;
        ____SPKR_DUTY____4      ; 23
        beq     :+              ; 
        lda     (audio_data)    ; 
        sta     next+1          ; 33

        WASTE_8                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_15                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $1100, error
duty_cycle17:
        ____SPKR_DUTY____4      ; 4
        lda     (audio_status)  ; 9
        and     #HAS_BYTE       ; 11
        WASTE_9                 ;
        ____SPKR_DUTY____4      ; 24
        beq     :+              ; 
        lda     (audio_data)    ; 
        sta     next+1          ; 34

        WASTE_7                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_14                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $1200, error
duty_cycle18:
        ____SPKR_DUTY____4      ; 4
        lda     (audio_status)  ; 9
        and     #HAS_BYTE       ; 11
        WASTE_10
        ____SPKR_DUTY____4      ; 25
        beq     :+              ; 
        lda     (audio_data)    ; 
        sta     next+1          ; 35

        WASTE_6                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_13                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $1300, error
duty_cycle19:
        ____SPKR_DUTY____4      ; 4
        lda     (audio_status)  ; 9
        and     #HAS_BYTE       ; 11
        WASTE_11
        ____SPKR_DUTY____4      ; 26
        beq     :+              ; 
        lda     (audio_data)    ; 
        sta     next+1          ; 36

        WASTE_5                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_12                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $1400, error
duty_cycle20:
        ____SPKR_DUTY____4      ; 4
        lda     (audio_status)  ; 9
        and     #HAS_BYTE       ; 11
        WASTE_12
        ____SPKR_DUTY____4      ; 27
        beq     :+              ; 
        lda     (audio_data)    ; 
        sta     next+1          ; 37

        WASTE_4                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_11                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $1500, error
duty_cycle21:
        ____SPKR_DUTY____4      ; 4
        lda     (audio_status)  ; 9
        and     #HAS_BYTE       ; 11
        WASTE_13
        ____SPKR_DUTY____4      ; 28
        beq     :+              ; 
        lda     (audio_data)    ; 
        sta     next+1          ; 38

        WASTE_3                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_10                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $1600, error
duty_cycle22:
        ____SPKR_DUTY____4      ; 4
        lda     (audio_status)  ; 9
        and     #HAS_BYTE       ; 11
        WASTE_14
        ____SPKR_DUTY____4      ; 29
        beq     :+              ; 
        lda     (audio_data)    ; 
        sta     next+1          ; 39

        WASTE_2                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_9                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $1700, error
duty_cycle23:
        ____SPKR_DUTY____4      ; 4
        lda     (audio_status)  ; 9
        and     #HAS_BYTE       ; 11
        beq     :+              ; 13/14
        lda     (audio_data)    ; 18
        WASTE_8                 ; 26
        ____SPKR_DUTY____4      ; 30
        sta     next+1          ; 33

        WASTE_8                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_12                ; 26
        ____SPKR_DUTY____4      ; 30
        WASTE_11                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)


.align 256
.assert * = _SAMPLES_BASE + $1800, error
duty_cycle24:
        ____SPKR_DUTY____4      ; 4
        lda     (audio_status)  ;
        and     #HAS_BYTE       ;
        beq     :+              ;
        lda     (audio_data)    ;
        WASTE_9                 ;
        ____SPKR_DUTY____4      ; 31
        sta     next+1          ;

        WASTE_7                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_13                ; 27
        ____SPKR_DUTY____4      ; 31
        WASTE_10                ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $1900, error
duty_cycle25:
        ____SPKR_DUTY____4      ; 4
        lda     (audio_status)  ;
        and     #HAS_BYTE       ;
        beq     :+              ;
        lda     (audio_data)    ;
        WASTE_10                ;
        ____SPKR_DUTY____4      ; 32
        sta     next+1          ;

        WASTE_6                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_14                ; 28
        ____SPKR_DUTY____4      ; 32
        WASTE_9                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $1A00, error
duty_cycle26:
        ____SPKR_DUTY____4      ; 4
        lda     (audio_status)  ;
        and     #HAS_BYTE       ;
        beq     :+              ;
        lda     (audio_data)    ;
        WASTE_11                ;
        ____SPKR_DUTY____4      ; 33
        sta     next+1          ;

        WASTE_5                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_15                ; 29
        ____SPKR_DUTY____4      ; 33
        WASTE_8                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $1B00, error
duty_cycle27:
        ____SPKR_DUTY____4      ; 4
        lda     (audio_status)  ;
        and     #HAS_BYTE       ;
        beq     :+              ;
        lda     (audio_data)    ;
        WASTE_12                ;
        ____SPKR_DUTY____4      ; 34
        sta     next+1          ;

        WASTE_4                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_16                ; 30
        ____SPKR_DUTY____4      ; 34
        WASTE_7                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $1C00, error
duty_cycle28:
        ____SPKR_DUTY____4      ; 4
        lda     (audio_status)  ;
        and     #HAS_BYTE       ;
        beq     :+              ;
        lda     (audio_data)    ;
        WASTE_13                ;
        ____SPKR_DUTY____4      ; 35
        sta     next+1          ;

        WASTE_3                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_17                ; 31
        ____SPKR_DUTY____4      ; 35
        WASTE_6                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $1D00, error
duty_cycle29:
        ____SPKR_DUTY____4      ; 4
        lda     (audio_status)  ;
        and     #HAS_BYTE       ;
        beq     :+              ;
        lda     (audio_data)    ;
        WASTE_14                ;
        ____SPKR_DUTY____4      ; 36
        sta     next+1          ;

        WASTE_2                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

:       WASTE_18                ; 32
        ____SPKR_DUTY____4      ; 36
        WASTE_5                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)

.align 256
.assert * = _SAMPLES_BASE + $1E00, error
duty_cycle30:
        ____SPKR_DUTY____4      ; 4
        lda     (audio_status)  ; 9
        and     #HAS_BYTE       ; 11
        beq     :+              ; 13/14
        lda     (audio_data)    ; 18
        sta     next+1          ; 21
        WASTE_9                 ; 30
        jmp     video_toggle_spkr ; 33          ; 90 (takes 49 cycles, jumps to next)

:       WASTE_19                ; 33
        ____SPKR_DUTY____4      ; 37
        WASTE_4                 ; 41
        jmp     video           ; 90 (takes 49 cycles, jumps to next)


.align 256
.assert * = _SAMPLES_BASE + $1F00, error
duty_cycle31:                   ; end spkr at 38
        ____SPKR_DUTY____4      ; 4
as31:   lda     $C0A9           ; 8
        and     #HAS_BYTE       ; 10
        beq     noa31           ; 12/13
ad31:   ldx     $C0A8           ; 16
        stx     next+1          ; 19
        bra     vs31            ; 22

noa31:  WASTE_9                 ; 22

vs31:   lda     $C099           ; 26
        and     #HAS_BYTE       ; 28
        beq     no_vid31        ; 30/31
vd31:   lda     $C098           ; 34

        ____SPKR_DUTY____4      ; 38
        WASTE_4                 ; 42
        jmp     video_direct    ; 45=>86 (takes 41 cycles, jumps to next)

no_vid31:
        WASTE_3                 ; 34
        ____SPKR_DUTY____4      ; 38
        WASTE_42                ; 80
        jmp     (next)          ; 86

.align 256
page1_addrs_arr:.res (N_BASES*2)          ; Base addresses arrays
.align 256                                ; Aligned for correct cycle counting
page2_addrs_arr:.res (N_BASES*2)

video_toggle_spkr:
        ____SPKR_DUTY____4      ; 37
        WASTE_4                 ; 41
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
        .byte   $9C ; stz ABSOLUTE to waste one cycle
        .byte   got_offset
        .byte   $00               ; 35
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
        WASTE_13                  ; 35
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

no_vid: 
        WASTE_38                  ; 47 - no video
        jmp     (next)            ; 53

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
        lda     (audio_status)
        and     #HAS_BYTE
        beq     silence
        lda     (audio_data)
        sta     start_duty+2
start_duty:
        jmp     $0000
; ------------------------------------------------------------------

; ------------------------------------------------------------------
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

        ; lda     #<($C098+1)
        ; sta     video_status+1
        ; lda     #>($C098+1)
        ; sta     video_status+2
        ; 
        ; lda     #<($C098)
        ; sta     video_data+1
        ; lda     #>($C098)
        ; sta     video_data+2

        lda     $C0A8+2
        sta     $C098+2
        lda     $C0A8+3
        sta     $C098+3

        rts

_pwm:
        pha
        ; Disable interrupts
        lda     #$00
        jsr     _simple_serial_set_irq

        pla
        ; Setup pointers
        jsr     setup_pointers

        ; Start with silence
        jmp     duty_cycle0

; ------------------------------------------------------------------

; .align 256
; .assert * = _SAMPLES_BASE+$2100, error
; break_out:
; .ifdef IIGS
;         lda     prevspd
;         jsr     _set_iigs_speed
; .endif
;         lda     #$01            ; Reenable IRQ and flush
;         jsr     _simple_serial_set_irq
;         jsr     _simple_serial_flush
;         lda     #$2F            ; SURL_CLIENT_READY
;         jmp     _serial_putc_direct


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

page_addr_ptr:  .byte <(page2_addr_ptr)   ; Base addresses pointer for page 2
                .byte <(page1_addr_ptr)   ; Base addresses pointer for page 1

        .bss
stop:           .res 1
dummy_abs:      .res 2
prevspd:        .res 1
