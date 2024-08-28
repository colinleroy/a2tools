; ---------------------------------------------------------
;
; Macros

; ease cycle counting
.macro WASTE_2                  ; Cycles wasters
        nop
.endmacro

.macro WASTE_3
        sta     wasted
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
        jsr waste_12
.endmacro

.macro WASTE_13
        WASTE_7
        WASTE_6
.endmacro

.macro WASTE_14
        jsr waste_14
.endmacro

.macro WASTE_15
        jsr waste_15
.endmacro

.macro WASTE_16
        jsr waste_16
.endmacro

.macro WASTE_17
        jsr waste_17
.endmacro

.macro WASTE_18
        jsr waste_18
.endmacro

.macro WASTE_19
        jsr waste_19
.endmacro

.macro WASTE_20
        jsr waste_20
.endmacro

.macro WASTE_21
        jsr waste_21
.endmacro

.macro WASTE_22
        jsr waste_22
.endmacro

.macro WASTE_23
        jsr waste_23
.endmacro

.macro WASTE_24
        jsr waste_24
.endmacro

.macro WASTE_25
        jsr waste_25
.endmacro

.macro WASTE_26
        jsr waste_26
.endmacro

.macro WASTE_27
        jsr waste_27
.endmacro

.macro WASTE_28
        jsr waste_28
.endmacro

.macro WASTE_29
        jsr waste_29
.endmacro

.macro WASTE_30
        jsr waste_30
.endmacro

.macro WASTE_31
        jsr waste_31
.endmacro

; Hack to waste 1 cycle. Use absolute stx with $00nn
.macro ABS_STX  zpvar
        .byte   $8E             ; stx absolute
        .byte   zpvar
        .byte   $00
.endmacro

; Hack to waste 1 cycle. Use absolute sta with $00nn
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
