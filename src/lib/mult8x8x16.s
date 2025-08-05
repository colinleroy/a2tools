
        .import         popa
        .export         _mult8x8r16
        .export         mult8x8r16_direct
        .include        "mult8x8x16_macro.inc"

_mult8x8r16:
        tax
        jsr     popa
        beq     zero

mult8x8r16_direct:
        MULT_AX
        rts

zero:
        tax
        rts
