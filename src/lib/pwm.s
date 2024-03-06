;
; Colin Leroy-Mira, 2024
;
        .export         _pwm
        .importzp       _zp6, _zp8, _zp10, _zp12, tmp1
        
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

spkr     =     $C030

data     =     $FFFF            ; Placeholders for legibility, going to be patched
status   =     $FFFF

spkr_ptr =     _zp6
dummy    =     _zp8
status_ptr =   _zp10
data_ptr   =   _zp12

        .segment        "DATA"

.align 256
BASE = *
.assert * = $4000, error
duty_cycle0:
        sta     spkr            ; 4  !
        sta     spkr            ; 8  !
        sta     dummy_dst       ; 12
        sta     dummy_dst       ; 16
        sta     dummy_dst       ; 20
        sta     dummy_dst       ; 24
        sta     tmp1            ; 27

s0:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d0:     lda     data            ; 39      - yes
        sta     dest0+2         ; 43
dest0:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle0     ;    46

.align 256
.assert * = BASE+$100, error
duty_cycle1:
        sta     spkr            ; 4  !
        sta     (spkr_ptr)      ; 9  !
        sta     dummy_dst       ; 13
        sta     dummy_dst       ; 17
        sta     dummy_dst       ; 21
        sta     dummy_dst       ; 25
        iny                     ; 27

s1:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d1:     lda     data            ; 39      - yes
        sta     dest1+2         ; 43
dest1:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle1     ;    46

.align 256
.assert * = BASE+$200, error
duty_cycle2:
        sta     spkr            ; 4  !
        iny                     ; 6  !
        sta     spkr            ; 10 !
        sta     dummy_dst       ; 14
        sta     dummy_dst       ; 18
        sta     dummy_dst       ; 22
        sta     (dummy)         ; 27

s2:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d2:     lda     data            ; 39      - yes
        sta     dest2+2         ; 43
dest2:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle2     ;    46

.align 256
.assert * = BASE+$300, error
duty_cycle3:
        sta     spkr            ; 4  !
        iny                     ; 6  !
        sta     (spkr_ptr)      ; 11 !
        sta     dummy_dst       ; 15
        sta     dummy_dst       ; 19
        sta     dummy_dst       ; 23
        sta     dummy_dst       ; 27

s3:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d3:     lda     data            ; 39      - yes
        sta     dest3+2         ; 43
dest3:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle3     ;    46


.align 256
.assert * = BASE+$400, error
duty_cycle4:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     spkr            ; 12 !
        sta     dummy_dst       ; 16
        sta     dummy_dst       ; 20
        sta     dummy_dst       ; 24
        sta     tmp1            ; 27

s4:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d4:     lda     data            ; 39      - yes
        sta     dest4+2         ; 43
dest4:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle4     ;    46


.align 256
.assert * = BASE+$500, error
duty_cycle5:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     (spkr_ptr)      ; 13 !
        sta     dummy_dst       ; 17
        sta     dummy_dst       ; 21
        sta     dummy_dst       ; 25
        iny                     ; 27

s5:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d5:     lda     data            ; 39      - yes
        sta     dest5+2         ; 43
dest5:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle5     ;    46


.align 256
.assert * = BASE+$600, error
duty_cycle6:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        iny                     ; 10 !
        sta     spkr            ; 14 !
        sta     dummy_dst       ; 18
        sta     dummy_dst       ; 22
        sta     (dummy)         ; 27

s6:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d6:     lda     data            ; 39      - yes
        sta     dest6+2         ; 43
dest6:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle6     ;    46


.align 256
.assert * = BASE+$700, error
duty_cycle7:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        iny                     ; 10 !
        sta     (spkr_ptr)      ; 15 !
        sta     dummy_dst       ; 19
        sta     dummy_dst       ; 23
        sta     dummy_dst       ; 27

s7:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d7:     lda     data            ; 39      - yes
        sta     dest7+2         ; 43
dest7:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle7     ;    46


.align 256
.assert * = BASE+$800, error
duty_cycle8:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     spkr            ; 16 !
        sta     dummy_dst       ; 20
        sta     dummy_dst       ; 24
        sta     tmp1            ; 27

s8:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d8:     lda     data            ; 39      - yes
        sta     dest8+2         ; 43
dest8:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle8     ;    46


.align 256
.assert * = BASE+$900, error
duty_cycle9:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     (spkr_ptr)      ; 17 !
        sta     dummy_dst       ; 21
        sta     dummy_dst       ; 25
        iny                     ; 27

s9:     lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d9:     lda     data            ; 39      - yes
        sta     dest9+2         ; 43
dest9:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle9     ;    46


.align 256
.assert * = BASE+$A00, error
duty_cycle10:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        iny                     ; 14 !
        sta     spkr            ; 18 !
        sta     dummy_dst       ; 22
        sta     tmp1            ; 25
        iny                     ; 27

s10:    lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d10:    lda     data            ; 39      - yes
        sta     dest10+2        ; 43
dest10:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle10    ;    46


.align 256
.assert * = BASE+$B00, error
duty_cycle11:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        iny                     ; 14 !
        sta     (spkr_ptr)      ; 19 !
        sta     dummy_dst       ; 23
        sta     dummy_dst       ; 27

s11:    lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d11:    lda     data            ; 39      - yes
        sta     dest11+2        ; 43
dest11:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle11    ;    46


.align 256
.assert * = BASE+$C00, error
duty_cycle12:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     spkr            ; 20 !
        sta     dummy_dst       ; 24
        sta     tmp1            ; 27

s12:    lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d12:    lda     data            ; 39      - yes
        sta     dest12+2        ; 43
dest12:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle12    ;    46


.align 256
.assert * = BASE+$D00, error
duty_cycle13:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     (spkr_ptr)      ; 21 !
        sta     dummy_dst       ; 25
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
        sta     dummy_dst       ;    42
        jmp     duty_cycle13    ;    46

.align 256
.assert * = BASE+$E00, error
duty_cycle14:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        iny                     ; 18 !
        sta     spkr            ; 22 !
        sta     (dummy)         ; 27

s14:    lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d14:    lda     data            ; 39      - yes
        sta     dest14+2        ; 43
dest14:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle14    ;    46


.align 256
.assert * = BASE+$F00, error
duty_cycle15:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        iny                     ; 18 !
        sta     (spkr_ptr)      ; 23 !
        sta     dummy_dst       ; 27

s15:    lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d15:    lda     data            ; 39      - yes
        sta     dest15+2        ; 43
dest15:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle15    ;    46


.align 256
.assert * = BASE+$1000, error
duty_cycle16:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     dummy_dst       ; 20 !
        sta     spkr            ; 24 !
        sta     tmp1            ; 27

s16:    lda     status          ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d16:    lda     data            ; 39      - yes
        sta     dest16+2        ; 43
dest16:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle16    ;    46


.align 256
.assert * = BASE+$1100, error
duty_cycle17:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     dummy_dst       ; 20 !
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
        sta     dummy_dst       ;    42
        jmp     duty_cycle17    ;    46


.align 256
.assert * = BASE+$1200, error
duty_cycle18:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     dummy_dst       ; 20 !
        iny                     ; 22 !
        sta     spkr            ; 26 !

        lda     (status_ptr)    ; 31      - have byte?
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d18:    lda     data            ; 39      - yes
        sta     dest18+2        ; 43
dest18:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle18    ;    46


.align 256
.assert * = BASE+$1300, error
duty_cycle19:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     dummy_dst       ; 20 !
        iny                     ; 22 !
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
        sta     dummy_dst       ;    42
        jmp     duty_cycle19    ;    46


.align 256
.assert * = BASE+$1400, error
duty_cycle20:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     dummy_dst       ; 20 !
        sta     (dummy)         ; 23 !

s20:    lda     status          ; 27 !      - have byte?
        sta     spkr            ; 31 !
        and     #HAS_BYTE       ; 33
        beq     :+              ; 35 36
d20:    lda     data            ; 39      - yes
        sta     dest20+2        ; 43
dest20:
        jmp     $0000           ; 46
:
        iny                     ;    38
        sta     dummy_dst       ;    42
        jmp     duty_cycle20    ;    46


.align 256
.assert * = BASE+$1500, error
duty_cycle21:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     (dummy)         ; 21 !

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
        sta     (dummy)         ;    43
        jmp     duty_cycle21    ;    46


.align 256
.assert * = BASE+$1600, error
duty_cycle22:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     dummy_dst       ; 20 !

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
        sta     spkr            ;    33 !
        sta     dummy_dst       ;    37
        sta     dummy_dst       ;    41
        iny                     ;    43
        jmp     duty_cycle22    ;    46


.align 256
.assert * = BASE+$1700, error
duty_cycle23:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     dummy_dst       ; 20 !

s23:    lda     status          ; 24 !      - have byte?
        and     #HAS_BYTE       ; 26 !
        beq     :+              ; 28 ! 30
        iny                     ; 30 !
        sta     spkr            ; 34 !
        lda     (data_ptr)      ; 39      - yes
        sta     dest23+2        ; 43
dest23:
        jmp     $0000           ; 46
:
        sta     spkr            ;    34 !
        sta     (dummy)         ;    39
        sta     dummy_dst       ;    43
        jmp     duty_cycle23    ;    46


.align 256
.assert * = BASE+$1800, error
duty_cycle24:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     dummy_dst       ; 20 !

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
        sta     dummy_dst       ;    39
        sta     dummy_dst       ;    43
        jmp     duty_cycle24    ;    46


.align 256
.assert * = BASE+$1900, error
duty_cycle25:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     dummy_dst       ; 20 !

s25:    lda     status          ; 24 !      - have byte?
        and     #HAS_BYTE       ; 26 !
        beq     :+              ; 28 ! 30
d25:    lda     data            ; 32 !     - yes
        iny                     ; 34 !
        sta     spkr            ; 36 !
        sta     dest25+2        ; 40
        sta     tmp1            ; 43
dest25:
        jmp     $0000           ; 46
:
        iny                     ;    32 !
        sta     spkr            ;    36 !
        sta     tmp1            ;    39
        sta     dummy_dst       ;    43
        jmp     duty_cycle25    ;    46


.align 256
.assert * = BASE+$1A00, error
duty_cycle26:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     dummy_dst       ; 20 !

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
        sta     dummy_dst       ;    43
        jmp     duty_cycle26    ;    46


.align 256
.assert * = BASE+$1B00, error
duty_cycle27:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     dummy_dst       ; 20 !

s27:    lda     status          ; 24 !      - have byte?
        and     #HAS_BYTE       ; 26 !
        beq     :+              ; 28 ! 30
d27:    lda     data            ; 32 !     - yes
        ldx     #$02            ; 34 !
        sta     spkr            ; 38 !
        sta     dest27,x        ; 43
dest27:
        jmp     $0000           ; 46
:
        sta     dummy_dst       ;    34 !
        sta     spkr            ;    38 !
        sta     (dummy)         ;    43
        jmp     duty_cycle27    ;    46


.align 256
.assert * = BASE+$1C00, error
duty_cycle28:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     dummy_dst       ; 20 !

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
        sta     dummy_dst       ;    34 !
        sta     (spkr_ptr)      ;    39 !
        sta     dummy_dst       ;    43
        jmp     duty_cycle28    ;    46


.align 256
.assert * = BASE+$1D00, error
duty_cycle29:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     dummy_dst       ; 20 !

s29:    lda     status          ; 24 !      - have byte?
        and     #HAS_BYTE       ; 26 !
        beq     :+              ; 28 ! 30
d29:    lda     data            ; 32 !     - yes
        sta     dest29+2        ; 36 !
        sta     spkr            ; 40 !
        bra     dest29          ; 43
dest29:
        jmp     $0000           ; 46
:
        sta     dummy_dst       ;    34 !
        iny                     ;    36 !
        sta     spkr            ;    40 !
        bra     :+              ;    43
:       jmp     duty_cycle29    ;    46


.align 256
.assert * = BASE+$1E00, error
duty_cycle30:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     dummy_dst       ; 20 !

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
        sta     dummy_dst       ;    34 !
        iny                     ;    36 !
        sta     (spkr_ptr)      ;    41 !
        iny                     ;    43
        jmp     duty_cycle30    ;    46


.align 256
.assert * = BASE+$1F00, error
duty_cycle31:
        sta     spkr            ; 4  !
        sta     dummy_dst       ; 8  !
        sta     dummy_dst       ; 12 !
        sta     dummy_dst       ; 16 !
        sta     dummy_dst       ; 20 !
        iny                     ; 22 !

s31:    lda     status          ; 26 !      - have byte?
        and     #HAS_BYTE       ; 28 !
        beq     :+              ; 30 ! 32
d31:    lda     data            ; 34 !     - yes
        sta     dest31+2        ; 38 !
        sta     (spkr_ptr)      ; 43 !
dest31:
        jmp     $0000           ; 46
:
        sta     dummy_dst       ;    36 !
        bra     :+              ;    39 !
:       sta     spkr            ;    43 !
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
        lda     #<(spkr)
        sta     spkr_ptr
        lda     #>(spkr)
        sta     spkr_ptr+1

        lda     #<(dummy_dst)
        sta     dummy
        lda     #>(dummy_dst)
        sta     dummy+1

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
        jsr     _get_iigs_speed
        sta     prevspd
        lda     #SPEED_SLOW
        jsr     _set_iigs_speed
.endif

        jmp     duty_cycle0

        .bss
dummy_dst: .res 1
counter:   .res 1
prevspd:   .res 1
