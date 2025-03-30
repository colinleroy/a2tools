; Copyright (C) 2025 Colin Leroy-Mira <colin@colino.net>
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program. If not, see <http://www.gnu.org/licenses/>.

        .export     _draw_screen
        .export     _move_puck, _move_my_pusher, _move_their_pusher
        .export     _puck_check_my_hit, _puck_check_their_hit
        .export     _opponent_think

        .export     puck_x, puck_y, puck_dx, puck_dy
        .export     my_pusher_x, my_pusher_y
        .export     their_pusher_x, their_pusher_y

        .export     _puck_reinit_my_order, _puck_reinit_their_order

        .import     _load_puck_pointer, _load_my_pusher_pointer, _load_their_pusher_pointer
        .import     _setup_sprite_pointer_full, _draw_sprite, _clear_sprite
        .import     _setup_sprite_pointer_for_clear
        .import     _setup_sprite_pointer_for_draw
        .import     mouse_x, mouse_y
        .import     mouse_dx, mouse_dy

        .import     x_shift, x_mult, y_transform

        .import     _my_pusher0, _my_pusher1, _my_pusher2, _my_pusher3
        .import     _their_pusher4, _their_pusher5
        .import     _puck0, _puck1, _puck2, _puck3, _puck4, _puck5, _puck6
        .import     tosumula0, pusha0

        .importzp   tmp1

        .include    "sprite.inc"
        .include    "puck_coords.inc"
        .include    "my_pusher_coords.inc"
        .include    "my_pusher0.gen.inc"
        .include    "their_pusher_coords.inc"
        .include    "their_pusher4.gen.inc"
        .include    "puck0.gen.inc"
        .include    "constants.inc"

.segment "LOWCODE"

.proc _draw_screen_my_side
        lda     frame_counter
        inc     frame_counter
        and     #1
        beq     :+

        ; Redraw other side
        jsr     _load_their_pusher_pointer
        jsr     _setup_sprite_pointer_full
        jsr     _clear_sprite
        jsr     _draw_sprite
        rts

:       ; ~ 12800 cycles
        jsr     _load_my_pusher_pointer
        jsr     _setup_sprite_pointer_for_clear
        jsr     _clear_sprite

        jsr     _load_puck_pointer
        jsr     _setup_sprite_pointer_full
        jsr     _clear_sprite
        jsr     _draw_sprite

        jsr     _load_my_pusher_pointer
        jsr     _setup_sprite_pointer_for_draw
        jsr     _draw_sprite
        rts
.endproc

.proc _draw_screen_their_side
        lda     frame_counter
        inc     frame_counter
        and     #1
        beq     :+

        ; ~6600 cycles - Their side
        jsr     _load_puck_pointer
        jsr     _setup_sprite_pointer_for_clear
        jsr     _clear_sprite

        jsr     _load_their_pusher_pointer
        jsr     _setup_sprite_pointer_full
        jsr     _clear_sprite
        jsr     _draw_sprite

        jsr     _load_puck_pointer
        jsr     _setup_sprite_pointer_for_draw
        jsr     _draw_sprite
        rts

:       ; My side now
        jsr     _load_my_pusher_pointer
        jsr     _setup_sprite_pointer_full
        jsr     _clear_sprite
        jsr     _draw_sprite
        rts
.endproc

.proc waste_3400
        ldy     #3
:       ldx     #226
:       dex                         ; 2
        bne     :-                  ; 5
        dey
        bne     :--
        rts
.endproc

; Draw screen, choosing which draw function to use depending
; on the puck's side.
.proc _draw_screen
        ;jsr     waste_3000           ; Test for 60Hz
        lda     puck_y
        cmp     #96                   ; Middle of HGR height

        bcs     :+
        jmp     _draw_screen_their_side
:       jmp     _draw_screen_my_side
.endproc
;X, Y input
;Updates X, Y, destroys A
.proc _transform_xy
        sty     tmpy
        lda     x_mult,y                ; Multiply if needed
        beq     :+
        stx     tmpx
        jsr     pusha0

        lda     tmpx
        jsr     tosumula0
:       txa                             ; /256 if we multiplied, X coord otherwise
        clc
        ldy     tmpy
        adc     x_shift,y               ; Add shift
        tax

        lda     y_transform,y           ; And transform Y
        tay
        rts
.endproc

my_pushers_low:
        .byte <_my_pusher0
        .byte <_my_pusher1
        .byte <_my_pusher2
        .byte <_my_pusher3
my_pushers_high:
        .byte >_my_pusher0
        .byte >_my_pusher1
        .byte >_my_pusher2
        .byte >_my_pusher3

; Y input
.proc _my_pusher_select
        ldx     #0
        cpy     #159
        bcs     out
        inx
        cpy     #147
        bcs     out
        inx
        cpy     #135
        bcs     out
        inx
out:
        lda     my_pushers_low,x
        sta     my_pusher_data+SPRITE_DATA::SPRITE
        lda     my_pushers_high,x
        sta     my_pusher_data+SPRITE_DATA::SPRITE+1
        rts
.endproc

; X,Y in input
.proc _move_my_pusher
        sty     my_pusher_y
        stx     my_pusher_x
        jsr     _transform_xy
        stx     my_pusher_gx
        tya
        sec
        sbc     #my_pusher0_HEIGHT
        tay
        sty     my_pusher_gy
        jmp     _my_pusher_select
.endproc

their_pushers_low:
        .byte <_their_pusher4
        .byte <_their_pusher5
their_pushers_high:
        .byte >_their_pusher4
        .byte >_their_pusher5

; Y input
.proc _their_pusher_select
        ldx     #0
        cpy     #75
        bcs     out
        inx
        cpy     #65
        bcs     out
        inx
out:
        lda     their_pushers_low,x
        sta     their_pusher_data+SPRITE_DATA::SPRITE
        lda     their_pushers_high,x
        sta     their_pusher_data+SPRITE_DATA::SPRITE+1
        rts
.endproc

; X,Y in input
.proc _move_their_pusher
        sty     their_pusher_y
        stx     their_pusher_x
        jsr     _transform_xy
        stx     their_pusher_gx
        tya
        sec
        sbc     #their_pusher4_HEIGHT
        tay
        sty     their_pusher_gy
        jmp     _their_pusher_select
.endproc


pucks_low:
        .byte <_puck0
        .byte <_puck1
        .byte <_puck2
        .byte <_puck3
        .byte <_puck4
        .byte <_puck5
        .byte <_puck6
pucks_high:
        .byte >_puck0
        .byte >_puck1
        .byte >_puck2
        .byte >_puck3
        .byte >_puck4
        .byte >_puck5
        .byte >_puck6

; Y input
.proc _puck_select
        ldx     #0
        cpy     #159
        bcs     out
        inx
        cpy     #141
        bcs     out
        inx
        cpy     #126
        bcs     out
        inx
        cpy     #111
        bcs     out
        inx
        cpy     #92
        bcs     out
        inx
        cpy     #80
        bcs     out
        inx
out:
        lda     pucks_low,x
        sta     puck_data+SPRITE_DATA::SPRITE
        lda     pucks_high,x
        sta     puck_data+SPRITE_DATA::SPRITE+1
        rts
.endproc

my_prev_puck_diff: .byte 0
their_prev_puck_diff: .byte 0

my_pusher_mid_x: .byte 0
my_currently_hitting: .byte 0
their_currently_hitting: .byte 0

.proc _puck_reinit_my_order
        lda     puck_y
        cmp     my_pusher_y
        lda     #0
        rol
        sta     my_prev_puck_diff
        rts
.endproc

.proc _puck_reinit_their_order
        lda     puck_y
        cmp     their_pusher_y
        lda     #0
        rol
        sta     their_prev_puck_diff
        rts
.endproc

.proc bind_puck_speed
        bit     puck_dy
        bmi     puck_backwards
puck_forwards:
        lda     puck_dy
        cmp     #ABS_MAX_DY
        bcc     bind_x
        lda     #ABS_MAX_DY
        sta     puck_dy
        jmp     bind_x
puck_backwards:
        lda     puck_dy
        clc
        eor     #$FF
        adc     #1
        cmp     #ABS_MAX_DY
        bcc     bind_x
        lda     #ABS_MAX_DY
        clc
        eor     #$FF
        adc     #1
        sta     puck_dy
bind_x:
        bit     puck_dx
        bmi     puck_left
puck_right:
        lda     puck_dx
        cmp     #ABS_MAX_DX
        bcc     out
        lda     #ABS_MAX_DX
        sta     puck_dx
        jmp     out
puck_left:
        lda     puck_dx
        clc
        eor     #$FF
        adc     #1
        cmp     #ABS_MAX_DX
        bcc     out
        lda     #ABS_MAX_DX
        clc
        eor     #$FF
        adc     #1
        sta     puck_dx
out:    clc                       ; Caller expects carry clear
        rts
.endproc

.proc _puck_check_my_hit
        ; Check if we already hit right before
        lda     my_currently_hitting
        beq     :+
        dec     my_currently_hitting
        jmp     _puck_reinit_my_order  ; Set puck/pusher order while it goes away

:       lda     puck_y
        cmp     my_pusher_y
        lda     #0
        rol
        cmp     my_prev_puck_diff
        bne     :+
        rts

        ; Order changed, check X
:       sta     my_prev_puck_diff

        lda     my_pusher_x
        cmp     puck_right_x
        bcs     out

        clc
        adc     #my_pusher0_WIDTH ; Same for their pusher, they are the same size
        bcs     :+                ; If adding our width overflowed, we're good
        cmp     puck_x            ; the pusher is on the right and the puck too
        bcc     out

:       ; Prevent multiple hits
        lda     #15
        sta     my_currently_hitting
        sta     $C030

        ; update puck speed
        ; Slow puck deltaX
        lda     puck_dx
        cmp     #$80
        ror
        beq     :+
        sta     puck_dx
:       lda     mouse_dx
        cmp     #$80
        ror
        cmp     #$80
        ror
        cmp     #$80
        ror
        clc
        adc     puck_dx
        sta     puck_dx

        ; Invert and slow puck delta-Y
        lda     puck_dy
        clc
        eor     #$FF
        adc     #$01
        sta     tmp1
        cmp     #$80
        ror
        bne     :+        ; But don't zero it
        lda     tmp1
:       sta     puck_dy
        lda     mouse_dy
        cmp     #$80
        ror
        cmp     #$80
        ror
        cmp     #$80
        ror
        clc
        adc     puck_dy
        sta     puck_dy

out:    jmp     bind_puck_speed
.endproc

.proc _puck_check_their_hit
        ; Check if we already hit right before
        lda     their_currently_hitting
        beq     :+
        dec     their_currently_hitting
        jmp     _puck_reinit_their_order  ; Set puck/pusher order while it goes away

:       lda     puck_y
        cmp     their_pusher_y
        lda     #0
        rol
        cmp     their_prev_puck_diff
        bne     :+
        rts

        ; Order changed, check X
:       sta     their_prev_puck_diff

        lda     their_pusher_x
        cmp     puck_right_x
        bcs     out_miss

        clc
        adc     #my_pusher0_WIDTH ; Same for their pusher, they are the same size
        bcs     :+                ; If adding our width overflowed, we're good
        cmp     puck_x            ; the pusher is on the right and the puck too
        bcc     out_miss

:       ; Prevent multiple hits
        lda     #15
        sta     their_currently_hitting
        sta     $C030

        ; update puck speed
        ; Slow puck deltaX
        lda     puck_dx
        cmp     #$80
        ror
        beq     :+
        sta     puck_dx
:       lda     their_pusher_dx
        cmp     #$80
        ror
        cmp     #$80
        ror
        cmp     #$80
        ror
        clc
        adc     puck_dx
        sta     puck_dx

        ; Invert and slow puck delta-Y
        lda     puck_dy
        clc
        eor     #$FF
        adc     #$01
        sta     tmp1
        cmp     #$80
        ror
        bne     :+        ; But don't zero it
        lda     tmp1
:       sta     puck_dy
        lda     their_pusher_dy
        cmp     #$80
        ror
        cmp     #$80
        ror
        cmp     #$80
        ror
        clc
        adc     puck_dy
        sta     puck_dy
        clc
        jmp     bind_puck_speed

out_miss:
        sec
        rts
.endproc

.proc revert_x
        lda     puck_dx
        clc
        eor     #$FF
        adc     #$01
        sta     puck_dx
        ; And back to move_puck
.endproc
.proc _move_puck
        bit     puck_dx
        bmi     puck_left

puck_right:
        lda     puck_x
        clc
        adc     puck_dx
        bcc     :+
        lda     #(PUCK_MAX_X+1)
        jmp     check_revert_x
puck_left:
        lda     puck_x
        clc
        adc     puck_dx
        bcs     :+
        lda     #<(PUCK_MIN_X-1)
:
check_revert_x:
        cmp     #(PUCK_MIN_X)
        bcc     revert_x
        cmp     #(PUCK_MAX_X)
        bcs     revert_x
        tax

        clc
        adc     #puck0_WIDTH
        bcc     :+                ; Don't let puck_right_x overflow
        lda     #$FF
:       sta     puck_right_x

update_y:
        bit     puck_dy
        bmi     puck_backwards

puck_forwards:
        lda     puck_y
        clc
        adc     puck_dy
        bcc     :+
        lda     #(PUCK_MAX_Y+1)
        jmp     check_y_bound

puck_backwards:
        lda     puck_y
        clc
        adc     puck_dy
        bcs     :+
        lda     #<(PUCK_MIN_Y-1)
:
check_y_bound:
        cmp     #(PUCK_MIN_Y)
        bcc     check_their_late_catch
        cmp     #(PUCK_MAX_Y)
        bcs     check_my_late_catch
        tay

        stx     puck_x
        sty     puck_y
        jsr     _transform_xy
        stx     puck_gx
        tya
        sec
        sbc     #puck0_HEIGHT
        tay
        sty     puck_gy

        jsr     _puck_select

        clc
        rts

crash:  sec
        rts

check_their_late_catch:
        lda     #PUCK_MIN_Y
        sta     puck_y
        jsr     _puck_check_their_hit
        bcc     update_y
        ; Or return with carry set to crash
        rts

check_my_late_catch:
        lda     #PUCK_MAX_Y
        sta     puck_y
        jsr     _puck_check_my_hit
        bcc     update_y
        ; Or return with carry set to crash
        rts

.endproc

.proc _opponent_think
        lda    their_pusher_x
        cmp    puck_x
        bcs    move_left

move_right:
        lda    puck_x
        sec
        sbc    their_pusher_x

        cmp    #THEIR_MAX_DX
        bcc    :+
        lda    #THEIR_MAX_DX
        clc
:
        sta    their_pusher_dx
        adc    their_pusher_x
        bcc    :+
        lda    #THEIR_PUSHER_MAX_X
:       cmp    #THEIR_PUSHER_MAX_X
        bcc    update_x
        lda    #THEIR_PUSHER_MAX_X
        jmp    update_x

move_left:
        lda    their_pusher_x
        sec
        sbc    puck_x
        sta    tmp1

        cmp    #THEIR_MAX_DX
        bcc    :+
        lda    #THEIR_MAX_DX
        sta    their_pusher_dx
        clc

:       lda    their_pusher_x
        sec
        sbc    tmp1
        php

        pha
        lda    tmp1
        clc
        adc    #$01
        eor    #$FF
        sta    their_pusher_dx
        pla

        plp
        bcs    update_x
        lda    #THEIR_PUSHER_MIN_X
update_x:
        sta    their_pusher_x

        lda    #THEIR_MAX_DY
        sta    their_pusher_dy
        rts
.endproc

.bss
tmpx:         .res 1
tmpy:         .res 1

puck_right_x: .res 1

puck_x:       .res 1
puck_y:       .res 1
my_pusher_x:  .res 1
my_pusher_y:  .res 1
their_pusher_x: .res 1
their_pusher_y: .res 1
their_pusher_dx: .res 1
their_pusher_dy: .res 1

puck_dx:      .res 1
puck_dy:      .res 1
frame_counter:.res 1
