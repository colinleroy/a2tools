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
