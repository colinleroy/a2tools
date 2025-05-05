; Macro'ed to avoid duplication but still have that code
; in opponents segments

.macro GET_DELTA_TO_PUCK
        lda     their_pusher_x
        clc                       ; Center puck on pusher
        adc     #((my_pusher0_WIDTH-puck0_WIDTH)/2)
        sta     mid_pusher+1
        ; Get the difference between puck and pusher
        lda     puck_x
        sec
mid_pusher:
        sbc     #$FF
.endmacro

.macro UNSIGNED_RAND_0_31_A
        jsr     _rand
        lsr
        lsr
        lsr
        clc
.endmacro

.macro UNSIGNED_RAND_0_15_A
        jsr     _rand
        lsr
        lsr
        lsr
        lsr
        clc
.endmacro

.macro UNSIGNED_RAND_0_7_A
        jsr     _rand
        lsr
        lsr
        lsr
        lsr
        lsr
        clc
.endmacro

.macro UNSIGNED_RAND_0_3_A
        jsr     _rand
        lsr
        lsr
        lsr
        lsr
        lsr
        lsr
        clc
.endmacro

.macro SIGNED_RAND_15_A
.scope
        UNSIGNED_RAND_0_15_A
        tay
        jsr     _rand
        and     #1
        beq     :+
        tya
        NEG_A
        tay
:       tya
.endscope
.endmacro

.macro SIGNED_RAND_0_15_A
.scope
        UNSIGNED_RAND_0_15_A
        tay
        jsr     _rand
        and     #1
        beq     :+
        tya
        NEG_A
        tay
:       tya
.endscope
.endmacro

.macro SIGNED_RAND_0_7_A
.scope
        UNSIGNED_RAND_0_7_A
        tay
        jsr     _rand
        and     #1
        beq     :+
        tya
        NEG_A
        tay
:       tya
.endscope
.endmacro

.macro SIGNED_RAND_0_3_A
.scope
        UNSIGNED_RAND_0_3_A
        tay
        jsr     _rand
        and     #1
        beq     :+
        tya
        NEG_A
        tay
:       tya
.endscope
.endmacro
