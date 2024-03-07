;
; Colin Leroy-Mira, 2024
;
        .export         _pwm
        .importzp       _zp6, _zp8, _zp10, _zp12, tmp1

        .import         _serial_putc_direct, _serial_read_byte_no_irq
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

        .segment        "DATA"

.align 256
BASE = *
.assert * = $8000, error
duty_cycle0:
        inc     dummy_abs       ; 6
        inc     dummy_abs       ; 12
        sta     dummy_abs       ; 16
        lda     KBD             ; 20
        bpl     :+              ; 23
        jsr     kbd_send
:       sta     dummy_abs       ; 27

s0:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d0:     lda     data            ; 39      - yes
        sta     dest0+2         ; 43
dest0:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle0     ;    46

.align 256
.assert * = BASE+$100, error
duty_cycle1:
        sta     SPKR            ; 4  !
        sta     spkr_ptr        ; 8  !
        sta     (dummy_ptr)     ; 13
        sta     dummy_abs       ; 17
        lda     KBD             ; 21
        bpl     :+              ; 24
        jsr     kbd_send
:       sta     dummy_zp        ; 27

s1:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d1:     lda     data            ; 39      - yes
        sta     dest1+2         ; 43
dest1:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle1     ;    46

.align 256
.assert * = BASE+$200, error
duty_cycle2:
        sta     SPKR            ; 4  !
        iny                     ; 6  !
        sta     SPKR            ; 10 !
        sta     dummy_abs       ; 14
        lda     KBD             ; 18
        bpl     :+              ; 21
        jsr     kbd_send
:       inc     dummy_abs       ; 27

s2:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d2:     lda     data            ; 39      - yes
        sta     dest2+2         ; 43
dest2:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle2     ;    46

.align 256
.assert * = BASE+$300, error
duty_cycle3:
        sta     SPKR            ; 4  !
        iny                     ; 6  !
        sta     (spkr_ptr)      ; 11 !
        lda     KBD             ; 15
        bpl     :+              ; 18
        jsr     kbd_send
:       sta     dummy_abs       ; 22
        sta     (dummy_ptr)     ; 27

s3:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d3:     lda     data            ; 39      - yes
        sta     dest3+2         ; 43
dest3:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle3     ;    46


.align 256
.assert * = BASE+$400, error
duty_cycle4:
        sta     SPKR            ; 4  !
        sta     dummy_abs       ; 8  !
        sta     SPKR            ; 12 !
        sta     dummy_abs       ; 16
        lda     KBD             ; 20
        bpl     :+              ; 23
        jsr     kbd_send
:       sta     dummy_abs       ; 27

s4:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d4:     lda     data            ; 39      - yes
        sta     dest4+2         ; 43
dest4:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle4     ;    46


.align 256
.assert * = BASE+$500, error
duty_cycle5:
        sta     SPKR            ; 4  !
        sta     dummy_abs       ; 8  !
        sta     (spkr_ptr)      ; 13 !
        lda     KBD             ; 17
        bpl     :+              ; 20
        jsr     kbd_send
:       sta     dummy_abs       ; 24
        sta     dummy_zp        ; 27

s5:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d5:     lda     data            ; 39      - yes
        sta     dest5+2         ; 43
dest5:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle5     ;    46


.align 256
.assert * = BASE+$600, error
duty_cycle6:
        sta     SPKR            ; 4  !
        inc     dummy_abs       ; 10 !
        sta     SPKR            ; 14 !
        lda     KBD             ; 18
        bpl     :+              ; 21
        jsr     kbd_send
:       inc     dummy_abs       ; 27

s6:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d6:     lda     data            ; 39      - yes
        sta     dest6+2         ; 43
dest6:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle6     ;    46


.align 256
.assert * = BASE+$700, error
duty_cycle7:
        sta     SPKR            ; 4  !
        inc     dummy_abs       ; 10 !
        sta     (spkr_ptr)      ; 15 !
        lda     KBD             ; 19
        bpl     :+              ; 22
        jsr     kbd_send
:       sta     (dummy_ptr)     ; 27

s7:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d7:     lda     data            ; 39      - yes
        sta     dest7+2         ; 43
dest7:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle7     ;    46


.align 256
.assert * = BASE+$800, error
duty_cycle8:
        sta     SPKR            ; 4  !
        sta     dummy_abs       ; 8  !
        sta     dummy_abs       ; 12 !
        sta     SPKR            ; 16 !
        lda     KBD             ; 20
        bpl     :+              ; 23
        jsr     kbd_send
:       sta     dummy_abs       ; 27

s8:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d8:     lda     data            ; 39      - yes
        sta     dest8+2         ; 43
dest8:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle8     ;    46


.align 256
.assert * = BASE+$900, error
duty_cycle9:
        sta     SPKR            ; 4  !
        sta     dummy_abs       ; 8  !
        sta     dummy_abs       ; 12 !
        sta     (spkr_ptr)      ; 17 !
        lda     KBD             ; 21
        bpl     :+              ; 24
        jsr     kbd_send
:       sta     dummy_zp        ; 27

s9:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d9:     lda     data            ; 39      - yes
        sta     dest9+2         ; 43
dest9:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle9     ;    46


.align 256
.assert * = BASE+$A00, error
duty_cycle10:
        sta     SPKR            ; 4  !
        sta     dummy_abs       ; 8  !
        inc     dummy_abs       ; 14 !
        sta     SPKR            ; 18 !
        lda     KBD             ; 22
        bpl     :+              ; 25
        jsr     kbd_send
:       iny                     ; 27

s10:    lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d10:    lda     data            ; 39      - yes
        sta     dest10+2        ; 43
dest10:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle10    ;    46


.align 256
.assert * = BASE+$B00, error
duty_cycle11:
        sta     SPKR            ; 4  !
        sta     dummy_abs       ; 8  !
        inc     dummy_abs       ; 14 !
        sta     (spkr_ptr)      ; 19 !
        lda     KBD             ; 23
        bpl     :+              ; 26
        jsr     kbd_send

:       lda     (status_ptr)    ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d11:    lda     data            ; 39      - yes
        sta     dest11+2        ; 43
dest11:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle11    ;    46


.align 256
.assert * = BASE+$C00, error
duty_cycle12:
        sta     SPKR            ; 4  !
        sta     dummy_abs       ; 8  !
        sta     dummy_abs       ; 12 !
        sta     dummy_abs       ; 16 !
        sta     SPKR            ; 20 !
        lda     KBD             ; 24
        bpl     :+              ; 27
        jsr     kbd_send
:
s12:    lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d12:    lda     data            ; 39      - yes
        sta     dest12+2        ; 43
dest12:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle12    ;    46


.align 256
.assert * = BASE+$D00, error
duty_cycle13:
        sta     SPKR            ; 4  !
        sta     dummy_abs       ; 8  !
        sta     dummy_abs       ; 12 !
        sta     dummy_abs       ; 16 !
        sta     (spkr_ptr)      ; 21 !
        sta     dummy_abs       ; 25
        iny                     ; 27

s13:    lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d13:    lda     data            ; 39      - yes
        sta     dest13+2        ; 43
dest13:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle13    ;    46

.align 256
.assert * = BASE+$E00, error
duty_cycle14:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       sta     dummy_abs       ; 15 !
        sta     dummy_zp        ; 18 !
        sta     SPKR            ; 22 !
        sta     (dummy_ptr)     ; 27

s14:    lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d14:    lda     data            ; 39      - yes
        sta     dest14+2        ; 43
dest14:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle14    ;    46


.align 256
.assert * = BASE+$F00, error
duty_cycle15:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       inc     dummy_abs       ; 17 !
        iny                     ; 19 !
        sta     SPKR            ; 23 !
        sta     dummy_abs       ; 27

s15:    lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d15:    lda     data            ; 39      - yes
        sta     dest15+2        ; 43
dest15:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle15    ;    46


.align 256
.assert * = BASE+$1000, error
duty_cycle16:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       sta     dummy_abs       ; 15 !
        sta     (dummy_ptr)     ; 20 !
        sta     SPKR            ; 24 !
        sta     dummy_zp        ; 27

s16:    lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d16:    lda     data            ; 39      - yes
        sta     dest16+2        ; 43
dest16:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle16    ;    46


.align 256
.assert * = BASE+$1100, error
duty_cycle17:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       inc     dummy_abs       ; 17 !
        sta     dummy_zp        ; 20 !
        sta     (spkr_ptr)      ; 25 !
        iny                     ; 27

s17:    lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d17:    lda     data            ; 39      - yes
        sta     dest17+2        ; 43
dest17:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle17    ;    46


.align 256
.assert * = BASE+$1200, error
duty_cycle18:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       inc     dummy_abs       ; 17 !
        sta     (dummy_ptr)     ; 22 !
        sta     SPKR            ; 26 !

        lda     (status_ptr)    ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d18:    lda     data            ; 39      - yes
        sta     dest18+2        ; 43
dest18:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle18    ;    46


.align 256
.assert * = BASE+$1300, error
duty_cycle19:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       inc     dummy_abs       ; 17 !
        sta     (dummy_ptr)     ; 22 !
        sta     (spkr_ptr)      ; 27 !

s19:    lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d19:    lda     data            ; 39      - yes
        sta     dest19+2        ; 43
dest19:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle19    ;    46


.align 256
.assert * = BASE+$1400, error
duty_cycle20:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       inc     dummy_abs       ; 17 !
        inc     dummy_abs       ; 23 !

s20:    lda     status          ; 27 !      - have byte?
        sta     SPKR            ; 31 !
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d20:    lda     data            ; 39      - yes
        sta     dest20+2        ; 43
dest20:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_abs       ;    42
        jmp     duty_cycle20    ;    46


.align 256
.assert * = BASE+$1500, error
duty_cycle21:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       inc     dummy_abs       ; 17 !
        sta     dummy_abs       ; 21 !

s21:    lda     status          ; 25 !      - have byte?
        and     #HAS_BYTE       ; 27 !
        sta     (spkr_ptr)      ; 32 !
        beq     :+              ; 34 35
        lda     (data_ptr)      ; 39      - yes
        sta     dest21+2        ; 43
dest21:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     (dummy_ptr)         ;    43
        jmp     duty_cycle21    ;    46


.align 256
.assert * = BASE+$1600, error
duty_cycle22:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       inc     dummy_abs       ; 17 !
        sta     dummy_zp        ; 20 !

s22:    lda     status          ; 24 !      - have byte?
        and     #HAS_BYTE       ; 26 !
        beq     :+              ; 28 ! 29
        sta     (spkr_ptr)      ; 33 !
        iny                     ; 35
d22:    lda     data            ; 39      - yes
        sta     dest22+2        ; 43
dest22:
        jmp     $0000           ; 46
:
        sta     SPKR            ;    33 !
        sta     dummy_abs       ;    37
        sta     dummy_abs       ;    41
        iny                     ;    43
        jmp     duty_cycle22    ;    46


.align 256
.assert * = BASE+$1700, error
duty_cycle23:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       inc     dummy_abs       ; 17 !
        sta     dummy_zp        ; 20 !

s23:    lda     status          ; 24 !      - have byte?
        and     #HAS_BYTE       ; 26 !
        beq     :+              ; 28 ! 30
        iny                     ; 30 !
        sta     SPKR            ; 34 !
        lda     (data_ptr)      ; 39      - yes
        sta     dest23+2        ; 43
dest23:
        jmp     $0000           ; 46
:
        sta     SPKR            ;    34 !
        sta     (dummy_ptr)     ;    39
        sta     dummy_abs       ;    43
        jmp     duty_cycle23    ;    46


.align 256
.assert * = BASE+$1800, error
duty_cycle24:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       inc     dummy_abs       ; 17 !
        sta     dummy_zp        ; 20 !

s24:    lda     status          ; 24 !      - have byte?
        and     #HAS_BYTE       ; 26 !
        beq     :+              ; 28 ! 30
        iny                     ; 30 !
        sta     (spkr_ptr)      ; 35 !
d24:    lda     data            ; 39      - yes
        sta     dest24+2        ; 43
dest24:
        jmp     $0000           ; 46
:
        sta     (spkr_ptr)      ;    35 !
        sta     dummy_abs       ;    39
        sta     dummy_abs       ;    43
        jmp     duty_cycle24    ;    46


.align 256
.assert * = BASE+$1900, error
duty_cycle25:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       inc     dummy_abs       ; 17 !
        sta     dummy_zp        ; 20 !

s25:    lda     status          ; 24 !      - have byte?
        and     #HAS_BYTE       ; 26 !
        beq     :+              ; 28 ! 30
d25:    lda     data            ; 32 !     - yes
        iny                     ; 34 !
        sta     SPKR            ; 36 !
        sta     dest25+2        ; 40
        sta     dummy_zp        ; 43
dest25:
        jmp     $0000           ; 46
:
        iny                     ;    32 !
        sta     SPKR            ;    36 !
        sta     dummy_zp        ;    39
        sta     dummy_abs       ;    43
        jmp     duty_cycle25    ;    46


.align 256
.assert * = BASE+$1A00, error
duty_cycle26:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       inc     dummy_abs       ; 17 !
        sta     dummy_zp        ; 20 !

s26:    lda     status          ; 24 !      - have byte?
        and     #HAS_BYTE       ; 26 !
        beq     :+              ; 28 ! 30
d26:    lda     data            ; 32 !     - yes
        iny                     ; 34 !
        sta     (spkr_ptr)      ; 37 !
        sta     dest26+2        ; 41
        iny                     ; 43
dest26:
        jmp     $0000           ; 46
:
        iny                     ;    32 !
        sta     (spkr_ptr)      ;    37 !
        iny                     ;    39
        sta     dummy_abs       ;    43
        jmp     duty_cycle26    ;    46


.align 256
.assert * = BASE+$1B00, error
duty_cycle27:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       inc     dummy_abs       ; 17 !
        sta     dummy_zp        ; 20 !

s27:    lda     status          ; 24 !      - have byte?
        and     #HAS_BYTE       ; 26 !
        beq     :+              ; 28 ! 30
d27:    lda     data            ; 32 !     - yes
        ldx     #$02            ; 34 !
        sta     SPKR            ; 38 !
        sta     dest27,x        ; 43
dest27:
        jmp     $0000           ; 46
:
        sta     dummy_abs       ;    34 !
        sta     SPKR            ;    38 !
        sta     (dummy_ptr)     ;    43
        jmp     duty_cycle27    ;    46


.align 256
.assert * = BASE+$1C00, error
duty_cycle28:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       inc     dummy_abs       ; 17 !
        sta     dummy_zp        ; 20 !

s28:    lda     status          ; 24 !      - have byte?
        and     #HAS_BYTE       ; 26 !
        beq     :+              ; 28 ! 30
d28:    lda     data            ; 32 !     - yes
        iny                     ; 34 !
        sta     (spkr_ptr)      ; 39 !
        sta     dest28+2        ; 43
dest28:
        jmp     $0000           ; 46
:
        sta     dummy_abs       ;    34 !
        sta     (spkr_ptr)      ;    39 !
        sta     dummy_abs       ;    43
        jmp     duty_cycle28    ;    46


.align 256
.assert * = BASE+$1D00, error
duty_cycle29:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       inc     dummy_abs       ; 17 !
        sta     dummy_zp        ; 20 !

s29:    lda     status          ; 24 !      - have byte?
        and     #HAS_BYTE       ; 26 !
        beq     :+              ; 28 ! 30
d29:    lda     data            ; 32 !     - yes
        sta     dest29+2        ; 36 !
        sta     SPKR            ; 40 !
        bra     dest29          ; 43
dest29:
        jmp     $0000           ; 46
:
        sta     dummy_abs       ;    34 !
        iny                     ;    36 !
        sta     SPKR            ;    40 !
        bra     :+              ;    43
:       jmp     duty_cycle29    ;    46


.align 256
.assert * = BASE+$1E00, error
duty_cycle30:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       inc     dummy_abs       ; 17 !
        sta     dummy_zp        ; 20 !

s30:    lda     status          ; 24 !      - have byte?
        and     #HAS_BYTE       ; 26 !
        beq     :+              ; 28 ! 30
d30:    lda     data            ; 32 !     - yes
        sta     dest30+2        ; 36 !
        sta     (spkr_ptr)      ; 41 !
        iny                     ; 43
dest30:
        jmp     $0000           ; 46
:
        sta     dummy_abs       ;    34 !
        iny                     ;    36 !
        sta     (spkr_ptr)      ;    41 !
        iny                     ;    43
        jmp     duty_cycle30    ;    46


.align 256
.assert * = BASE+$1F00, error
duty_cycle31:
        sta     SPKR            ; 4  !
        lda     KBD             ; 8  !
        bpl     :+              ; 11 !
        jsr     kbd_send        ;    !
:       inc     dummy_abs       ; 17 !
        sta     (dummy_ptr)         ; 22 !

s31:    lda     status          ; 26 !      - have byte?
        and     #HAS_BYTE       ; 28 !
        beq     :+              ; 30 ! 32
d31:    lda     data            ; 34 !     - yes
        sta     dest31+2        ; 38 !
        sta     (spkr_ptr)      ; 43 !
dest31:
        jmp     $0000           ; 46
:
        sta     dummy_abs       ;    36 !
        bra     :+              ;    39 !
:       sta     SPKR            ;    43 !
        jmp     duty_cycle31    ;    46

.align 256
.assert * = BASE+$2000, error
break_out:
.ifdef IIGS
        lda     prevspd
        jsr     _set_iigs_speed
.endif
        rts

_pwm:
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
        ; sta     s11+1
        sta     s12+1
        sta     s13+1
        sta     s14+1
        sta     s15+1
        sta     s16+1
        sta     s17+1
        ; sta     s18+1
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
        ; sta     s11+2
        sta     s12+2
        sta     s13+2
        sta     s14+2
        sta     s15+2
        sta     s16+2
        sta     s17+2
        ; sta     s18+2
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
        ; sta     d21+1
        sta     d22+1
        ; sta     d23+1
        sta     d24+1
        sta     d25+1
        sta     d26+1
        sta     d27+1
        sta     d28+1
        sta     d29+1
        sta     d30+1
        sta     d31+1

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
        ; sta     d21+2
        sta     d22+2
        ; sta     d23+2
        sta     d24+2
        sta     d25+2
        sta     d26+2
        sta     d27+2
        sta     d28+2
        sta     d29+2
        sta     d30+2
        sta     d31+2

.ifdef IIGS
        ; Slow down IIgs
        jsr     _get_iigs_speed
        sta     prevspd
        lda     #SPEED_SLOW
        jsr     _set_iigs_speed
.endif
        ; Start with silence
        jmp     duty_cycle0

kbd_send:
        and     #$7F            ; Clear high bit
        bit     KBDSTRB         ; Clear keyboard strobe
        jmp     _serial_putc_direct

        .bss
dummy_abs: .res 1
counter:   .res 1
prevspd:   .res 1
minutes:   .res 1
seconds:   .res 1
