
        .export         sqrLow, sqrHigh, invLow, invHigh

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

invLow:
    .byte 0
    .repeat 255, I
        .byte <(65536/(I+1))
    .endrep

invHigh:
    .byte 0
    .repeat 255, I
        .byte >(65536/(I+1))
    .endrep
