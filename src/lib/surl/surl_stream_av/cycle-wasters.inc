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

.macro WASTE_45
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
