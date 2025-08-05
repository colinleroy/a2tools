
        .export         sqrLow, sqrHigh
        .export         iSqrLow, iSqrHigh
        .export         invLow, invHigh

.segment        "DATA"

; Tables must be aligned with page boundary
.align 256
sqrLow:
    .repeat 512, I
        .byte <((I*I)/4)
    .endrep

.align 256
sqrHigh:
    .repeat 512, I
        .byte >((I*I)/4)
    .endrep

.align 256
iSqrLow:
    .repeat 512, I
        .byte <(((I-255)*(I-255))/4)
    .endrepeat

.align 256
iSqrHigh:
    .repeat 512, I
        .byte >(((I-255)*(I-255))/4)
    .endrepeat

.align 256
invLow:
    .byte 0
    .repeat 255, I
        .byte <(65536/(I+1))
    .endrep

.align 256
invHigh:
    .byte 0
    .repeat 255, I
        .byte >(65536/(I+1))
    .endrep
