.macro NEG_A
        clc
        eor     #$FF
        adc     #1
.endmacro

.macro ADD_SIGNED_BOUNDS signed_add, min, max
.scope
        bit     signed_add
        bmi     bind_min
bind_max:
        adc     signed_add
        bcc     :+
        lda     max
:       cmp     max
        bcc     out
        lda     max
        jmp     out

bind_min:
        adc     signed_add
        bcs     :+
        lda     min
:       cmp     min
        bcs     out
        lda     min
out:
.endscope
.endmacro

.macro BIND_SIGNED maximum
.scope
        bmi     negative
positive:
        cmp     maximum
        bcc     out
        lda     maximum
        jmp     out
negative:
        NEG_A
        cmp     maximum
        bcc     :+            ; Restore sign
        lda     maximum
:       NEG_A
out:
.endscope
.endmacro
