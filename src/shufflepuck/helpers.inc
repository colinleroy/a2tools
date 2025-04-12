.macro NEG_A
        clc
        eor     #$FF
        adc     #1
.endmacro
