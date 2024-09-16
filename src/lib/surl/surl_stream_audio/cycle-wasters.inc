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
        WASTE_2
        WASTE_2
.endmacro

.macro WASTE_5
      WASTE_2
      WASTE_3
.endmacro

.macro WASTE_6
        WASTE_2
        WASTE_4
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
        WASTE_4
        WASTE_5
.endmacro

.macro WASTE_10
        WASTE_4
        WASTE_6
.endmacro

.macro WASTE_11
        WASTE_6
        WASTE_5
.endmacro

.macro WASTE_12
        jsr     waste
.endmacro
waste:          rts

.macro KBD_LOAD_7               ; Check keyboard and jsr if key pressed (trashes A)
        lda     KBD             ; 4
        bpl     :+              ; 7
        jsr     kbd_send
:
.endmacro
