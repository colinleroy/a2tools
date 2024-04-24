  .export _read_stuff, _read_stuff_nocheck
  .importzp tmp1
  .import _simple_serial_set_irq

.macro WASTE_3
        stz     tmp1
.endmacro

.macro WASTE_10
        nop
        nop
        nop
        nop
        nop
.endmacro

.macro WASTE_63
        WASTE_10
        WASTE_10
        WASTE_10
        WASTE_10
        WASTE_10
        WASTE_10
        WASTE_3
.endmacro

.macro WASTE_59
        WASTE_10
        WASTE_10
        WASTE_10
        WASTE_10
        WASTE_10
        WASTE_3
        WASTE_3
        WASTE_3
.endmacro

_read_stuff:
        lda     #$00
        jsr     _simple_serial_set_irq

:       lda     $C0A9
        and     #$08
        beq     :-
        lda     $C0A8
        cmp     #$F0
        beq     :-
        cmp     #$0F
        beq     :-
        ldx     #$00
        rts

_read_stuff_nocheck:
        lda     #$00            ; disable IRQ
        jsr     _simple_serial_set_irq
        php
        sei

:       lda     $C0A9           ; Wait for first byte
        and     #$08
        beq     :-

        ; loop, reading data reg directly
duty_cycle:
        lda     $C0A8           ; 4
        cmp     #$F0            ; 6
        beq     ok_f0           ; 8/9
        cmp     #$0F            ; 10
        beq     ok_0f           ; 12/13

        plp                     ; We got something else. Return it.
        ldx     #$00
        rts

ok_f0:  WASTE_63                ; 72
        bra     duty_cycle      ; 75

ok_0f:  WASTE_59                ; 72
        bra     duty_cycle      ; 75
