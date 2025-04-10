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

        .export     _draw_screen, _clear_screen
        .export     _move_puck, _move_my_pusher, _move_their_pusher
        .export     _puck_check_my_hit, _puck_check_their_hit
        .export     _puck_select

        .export     puck_x, puck_right_x, puck_y, puck_dx, puck_dy
        .export     _init_precise_x, _init_precise_y
        .export     my_pusher_x, my_pusher_y
        .export     their_pusher_x, their_pusher_y
        .export     their_pusher_dx, their_pusher_dy
        .export     their_currently_hitting, player_caught

        .export     _puck_reinit_my_order, _puck_reinit_their_order
        .export      puck_in_front_of_me, puck_in_front_of_them
        .export      prev_puck_in_front_of_me, prev_puck_in_front_of_them

        .export     _transform_puck_coords

        .export     _guess_puck_x_at_y

        .import     _setup_sprite_pointer_full, _draw_sprite, _clear_sprite
        .import     _setup_sprite_pointer_for_clear
        .import     _setup_sprite_pointer_for_draw
        .import     _setup_eor_draw, _setup_eor_clear, _draw_eor, _clear_eor
        .import     mouse_x, mouse_y
        .import     mouse_dx, mouse_dy
        .import     _crash_lines_scale, _draw_crash_lines
        .import     my_score, their_score

        .import     hz

        .import     x_shift, x_factor, y_factor

        .import     umul8x8r16, _platform_msleep

        .import     _play_puck_hit
        .import     _play_puck
        .import     _play_crash

        .import     my_pushers_low, my_pushers_high, my_pushers_width, my_pushers_height, my_pushers_bytes, my_pushers_bpline
        .import     their_pushers_low, their_pushers_high, their_pushers_width, their_pushers_height, their_pushers_bytes, their_pushers_bpline
        .import     pucks_low, pucks_high, pucks_width, pucks_height, pucks_bytes, pucks_bpline

        .importzp   tmp1, tmp3, ptr1

        .include    "sprite.inc"
        .include    "puck_coords.inc"
        .include    "my_pusher_coords.inc"
        .include    "their_pusher_coords.inc"
        .include    "puck0.gen.inc"
        .include    "my_pusher0.gen.inc"
        .include    "constants.inc"
        .include    "opponent_file.inc"
        .include    "hgr_applesoft.inc"

.segment "CODE"

.proc _clear_screen
        jsr     clear_their_pusher
        jsr     clear_my_pusher
        jsr     clear_puck
        rts
.endproc

.proc clear_puck
        lda     #<puck_data
        ldx     #>puck_data
        jsr     _setup_sprite_pointer_for_clear
        jmp     _clear_sprite
.endproc

.proc draw_puck
        jsr     _puck_select
        lda     #<puck_data
        ldx     #>puck_data
        jsr     _setup_sprite_pointer_for_draw
        jmp     _draw_sprite
.endproc

.proc clear_my_pusher
        lda     #<my_pusher_data
        ldx     #>my_pusher_data
        jsr     _setup_eor_clear
        jmp     _clear_eor
.endproc

.proc draw_my_pusher
        jsr     _my_pusher_select
        lda     #<my_pusher_data
        ldx     #>my_pusher_data
        jsr     _setup_eor_draw
        jmp     _draw_eor
.endproc

.proc clear_their_pusher
        lda     #<their_pusher_data
        ldx     #>their_pusher_data
        jsr     _setup_sprite_pointer_for_clear
        jmp     _clear_sprite
.endproc

.proc draw_their_pusher
        jsr     _their_pusher_select
        lda     #<their_pusher_data
        ldx     #>their_pusher_data
        jsr     _setup_sprite_pointer_for_draw
        jmp     _draw_sprite
.endproc

.segment "LOWCODE"

.proc _draw_screen_my_side
        ; Redraw their side first (it's higher on the screen)
        jsr     clear_their_pusher
        jsr     draw_their_pusher

        ; Redraw my side, taking care of the puck/pusher order
        lda     prev_puck_in_front_of_me
        beq     :+

        jsr     clear_puck
        jsr     clear_my_pusher
        jmp     draw

:       jsr     clear_my_pusher
        jsr     clear_puck

draw:
        lda     puck_in_front_of_me
        sta     prev_puck_in_front_of_me
        bne     :+

        jsr     draw_puck
        jsr     draw_my_pusher
        jmp     out

:       jsr     draw_my_pusher
        jsr     draw_puck

out:
        rts
.endproc

.proc _draw_screen_their_side
        ; Redraw their side first (it's higher on the screen)
        lda     prev_puck_in_front_of_them
        beq     :+

        jsr     clear_puck
        jsr     clear_their_pusher
        jmp     draw

:       jsr     clear_their_pusher
        jsr     clear_puck

draw:
        lda     puck_in_front_of_them
        sta     prev_puck_in_front_of_them
        bne     :+

        jsr     draw_puck
        jsr     draw_their_pusher
        jmp     my_side

:       jsr     draw_their_pusher
        jsr     draw_puck

my_side:
        ; My side now
        jsr     clear_my_pusher
        jsr     draw_my_pusher
        rts
.endproc

.proc waste_3400
        lda     hz
        cmp     #60
        bne     :+
        rts
:
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
; ~ 12400 cycles
.proc _draw_screen
        jsr     waste_3400           ; Test for 60Hz
        lda     puck_y

        cmp     #MID_BOARD           ; Middle of HGR height

        bcs     :+
        jmp     _draw_screen_their_side
:       jmp     _draw_screen_my_side
.endproc

;X, Y input
;Updates X, Y, destroys A
.proc _transform_xy
        sty     tmpy
        lda     x_factor,y                ; Multiply if needed
        beq     :+
        stx     tmpx

        sta     ptr1

        lda     tmpx
        jsr     umul8x8r16
:       txa                             ; /256 if we multiplied, X coord otherwise
        clc
        ldy     tmpy
        adc     x_shift,y               ; Add shift
        tax

        lda     y_factor,y           ; And transform Y
        tay
        rts
.endproc

; Y input
.proc _my_pusher_select
        ldy     my_pusher_y
        ldx     #0
        cpy     #183
        bcs     out
        inx
        cpy     #174
        bcs     out
        inx
        cpy     #165
        bcs     out
        inx
out:
        lda     my_pushers_low,x
        sta     my_pusher_data+SPRITE_DATA::SPRITE
        lda     my_pushers_high,x
        sta     my_pusher_data+SPRITE_DATA::SPRITE+1
        lda     my_pushers_bytes,x
        sta     my_pusher_data+SPRITE_DATA::BYTES
        lda     my_pushers_bpline,x
        sta     my_pusher_data+SPRITE_DATA::BYTES_WIDTH

        lda     my_pusher_gy
        sec
        sbc     my_pushers_height,x
        sta     my_pusher_gy
        rts
.endproc

; X,Y in input
.proc _move_my_pusher
        ldy     mouse_y
        sty     my_pusher_y
        ldx     mouse_x
        stx     my_pusher_x
        jsr     _transform_xy
        stx     my_pusher_gx
        sty     my_pusher_gy
        rts
.endproc

; Y input
.proc _their_pusher_select
        ldy     their_pusher_y
        ldx     #0
        cpy     #20
        bcs     out
        inx
out:
        lda     their_pushers_low,x
        sta     their_pusher_data+SPRITE_DATA::SPRITE
        lda     their_pushers_high,x
        sta     their_pusher_data+SPRITE_DATA::SPRITE+1
        lda     their_pushers_bytes,x
        sta     their_pusher_data+SPRITE_DATA::BYTES
        lda     their_pushers_bpline,x
        sta     their_pusher_data+SPRITE_DATA::BYTES_WIDTH

        lda     their_pusher_gy
        sec
        sbc     their_pushers_height,x
        sta     their_pusher_gy
        rts
.endproc

; X,Y in input
.proc _move_their_pusher
        clc
        bit     their_pusher_dx
        bmi     move_left
move_right:
        lda     their_pusher_x
        adc     their_pusher_dx
        bcc     :+
        lda     #THEIR_PUSHER_MAX_X
:       cmp     #THEIR_PUSHER_MAX_X
        bcc     store_x
        lda     #THEIR_PUSHER_MAX_X
        jmp     store_x

move_left:
        lda     their_pusher_x
        adc     their_pusher_dx
        bcs     :+
        lda     #THEIR_PUSHER_MIN_X
:       cmp     #THEIR_PUSHER_MIN_X
        bcs     store_x
        lda     #THEIR_PUSHER_MIN_X

store_x:
        sta     their_pusher_x
        tax

        ; Now update Y
        clc
        bit     their_pusher_dy
        bmi     move_backwards

move_forwards:
        lda     their_pusher_y
        adc     their_pusher_dy
        cmp     #THEIR_PUSHER_MAX_Y
        bcc     :+
        lda     #THEIR_PUSHER_MAX_Y
:       sta     their_pusher_y
        jmp     do_move

move_backwards:
        lda     their_pusher_y
        adc     their_pusher_dy
        cmp     #THEIR_PUSHER_MIN_Y
        bcs     :+
        lda     #THEIR_PUSHER_MIN_Y+1
:       sta     their_pusher_y

do_move:
        tay

        jsr     _transform_xy
        stx     their_pusher_gx
        sty     their_pusher_gy
        rts
.endproc

; Y input
.proc _puck_select
        ldy     puck_y
        ldx     #0
        cpy     #175
        bcs     out
        inx
        cpy     #148
        bcs     out
        inx
        cpy     #121
        bcs     out
        inx
        cpy     #94
        bcs     out
        inx
        cpy     #67
        bcs     out
        inx
        cpy     #40
        bcs     out
        inx
out:
        lda     pucks_low,x
        sta     puck_data+SPRITE_DATA::SPRITE
        lda     pucks_high,x
        sta     puck_data+SPRITE_DATA::SPRITE+1
        lda     pucks_bytes,x
        sta     puck_data+SPRITE_DATA::BYTES
        lda     pucks_bpline,x
        sta     puck_data+SPRITE_DATA::BYTES_WIDTH

        lda     puck_gy
        sec
        sbc     pucks_height,x
        sta     puck_gy
        rts
.endproc

puck_in_front_of_me: .byte 0
puck_in_front_of_them: .byte 0

prev_puck_in_front_of_me: .byte 0
prev_puck_in_front_of_them: .byte 0

my_pusher_mid_x: .byte 0
my_currently_hitting: .byte 0
their_currently_hitting: .byte 0

.proc _puck_reinit_my_order
        lda     puck_y
        cmp     my_pusher_y
        lda     #0
        rol
        sta     puck_in_front_of_me
        rts
.endproc

.proc _puck_reinit_their_order
        lda     puck_y
        cmp     their_pusher_y
        lda     #0
        rol
        sta     puck_in_front_of_them
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
        beq     check
        dec     my_currently_hitting

check:
        lda     puck_y
        cmp     my_pusher_y
        lda     #0
        rol
        cmp     puck_in_front_of_me
        bne     :+
        rts

        ; Order changed, store it
:       sta     puck_in_front_of_me

        ; Compare X
        lda     my_pusher_x
        cmp     puck_right_x
        bcs     out

        clc
        adc     #my_pusher0_WIDTH ; Same for their pusher, they are the same size
        bcs     :+                ; If adding our width overflowed, we're good
        cmp     puck_x            ; the pusher is on the right and the puck too
        bcc     out

:       ; Prevent multiple hits
        lda     #1
        sta     my_currently_hitting

        ; Make sure puck doesn't go behind pusher
        ldy     my_pusher_y
        dey
        tya
        sty     puck_y
        jsr     _init_precise_y

        jsr     _puck_reinit_my_order  ; Set puck/pusher order while it goes away
        ; update puck speed
        ; Slow puck deltaX
        lda     puck_dx
        cmp     #$80
        ror
        cmp     #$80
        ror
        beq     :+
        sta     puck_dx
:       lda     mouse_dx
        adc     puck_dx
        sta     puck_dx

        ; Invert and slow puck delta-Y if incoming
        lda     puck_dy
        bmi     :+
        jsr     revert_y

        lda     #1
        sta     player_caught     ; Used to inform opponents about the fact we caught the puck
                                  ; It's the opponent's responsability to zero it once they
                                  ; know

        ; And play sound
        ldy     #0
        jsr     _play_puck_hit

        ; Add our delta-Y to the puck
:       lda     mouse_dy
        adc     puck_dy
        bmi     :+                ; Don't let DY still be positive or zero
        lda     #$FF
:       sta     puck_dy

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
        cmp     puck_in_front_of_them
        bne     :+
        rts

        ; Order changed, check X
:       sta     puck_in_front_of_them

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
        ldy     #4
        jsr     _play_puck_hit

        ; Make sure puck doesn't go behind pusher
        ldy     their_pusher_y
        iny
        tya
        sty     puck_y
        jsr     _init_precise_y



        ; update puck speed
        ; Slow puck deltaX
        lda     puck_dx
        cmp     #$80
        ror
        cmp     #$80
        ror
        beq     :+
        sta     puck_dx
:       lda     their_pusher_dx
        cmp     #$80
        ror
        clc
        adc     puck_dx
        sta     puck_dx

        ; Invert and slow puck delta-Y
        jsr     revert_y
        lda     their_pusher_dy
        cmp     #$80
        ror
        clc
        adc     puck_dy
        cmp     #$01
        bcs     :+
        lda     #$01
:       sta     puck_dy
        clc
        jmp     bind_puck_speed

out_miss:
        sec
        rts
.endproc

.proc play_revert_x
        lda     #PUCK_MAX_Y
        sec
        sbc     puck_y
        lsr
        lsr
        lsr
        lsr
        lsr
        tay
        jmp     _play_puck
.endproc

.proc _transform_puck_coords
        ldx     puck_x
        ldy     puck_y
        jsr     _transform_xy
        stx     puck_gx
        sty     puck_gy
        rts
.endproc

.assert puck_precise_x = puck_x + 1, error
.assert puck_dx = puck_precise_x + 2, error
.assert puck_y = puck_dx + 2, error
.assert puck_precise_y = puck_y + 1, error
.assert puck_dy = puck_precise_y + 2, error

; Desired Y in A, result in A
.proc _guess_puck_x_at_y
        sta     tmp1

        ; Backup variables
        ldy     #9
:       lda     puck_x,y
        sta     puck_backup,y
        dey
        bpl     :-

        lda     #0
        sta     check_hits

:       lda     puck_y
        cmp     tmp1
        beq     out
        bcc     out

        jsr     _move_puck
        jmp     :-

out:
        lda     #1
        sta     check_hits


        ldx     puck_x
        ; Restore variables
        ldy     #9
:       lda     puck_backup,y
        sta     puck_x,y
        dey
        bpl     :-

        txa
        rts
.endproc

check_hits: .byte 1

.proc revert_y
        lda     puck_dy
        clc
        eor     #$FF
        adc     #$01
        cmp     #$80
        ror
        cmp     #$80
        ror
        cmp     #$80
        ror
        sta     puck_dy
        rts
.endproc

.proc revert_x
        lda     puck_dx
        clc
        eor     #$FF
        adc     #$01
        sta     puck_dx
        lda     check_hits
        beq     :+
        jsr     play_revert_x
:
        ; And back to move_puck
.endproc
.proc _move_puck
        ; Extend puck_dx to 16bits
        ldx     #$00
        lda     puck_dx
        bpl     :+
        dex
:       stx     puck_dx+1
        ; Update precise X coord
        lda     puck_precise_x
        clc
        adc     puck_dx
        sta     puck_precise_x
        lda     puck_precise_x+1
        adc     puck_dx+1
        sta     puck_precise_x+1

        tax                       ; Save high byte to X
        lsr                       ; Divide by two
        lda     puck_precise_x
        ror

        ; Check X bound
        cpx     #$FF
        beq     revert_x
        cpx     #$02
        bcs     revert_x
        cmp     #PUCK_MAX_X
        bcs     revert_x
        sta     puck_x

        clc
        adc     #puck0_WIDTH
        bcc     :+                ; Don't let puck_right_x overflow
        lda     #$FF
:       sta     puck_right_x

update_y:
        ; Extend puck_dy to 16bits
        ldx     #$00
        lda     puck_dy
        bpl     :+
        dex
:       stx     puck_dy+1
        ; Update precise Y coord
        lda     puck_precise_y
        clc
        adc     puck_dy
        sta     puck_precise_y
        lda     puck_precise_y+1
        adc     puck_dy+1
        sta     puck_precise_y+1

check_y_bound:
        tax                       ; Save high byte to X
        lsr                       ; Divide by two
        lda     puck_precise_y
        ror
        sta     puck_y

        ldy     check_hits        ; Are we simulating?
        beq     out

        ; Check opponent bound
        cpx     #$FF
        beq     check_their_late_catch
        cmp     #PUCK_MIN_Y
        bcc     check_their_late_catch

        ; Check our bound
        cmp     #PUCK_MAX_Y
        bcs     check_my_late_catch

        jsr     _transform_puck_coords

        clc
out:    rts

check_their_late_catch:
        lda     #PUCK_MIN_Y
        sta     puck_y
        jsr     _init_precise_y

        jsr     _puck_check_their_hit
        bcc     update_y
        jsr     __OPPONENT_START__+OPPONENT::LOSE_POINT
        clc                       ; Little crash
        jsr     update_screen_for_crash
        ldy     #4
        jsr     _play_crash
        lda     #200
        ldx     #0
        jsr     _platform_msleep
        jsr     __OPPONENT_START__+OPPONENT::LOSE_POINT_SND
        ; Return with carry set to inform main
        sec
        inc     my_score
        rts

check_my_late_catch:
        lda     #PUCK_MAX_Y
        sta     puck_y
        jsr     _init_precise_y

        jsr     _puck_check_my_hit
        bcc     update_y
        jsr     __OPPONENT_START__+OPPONENT::WIN_POINT
        sec                       ; Large crash
        jsr     update_screen_for_crash
        ldy     #0
        jsr     _play_crash
        lda     #200
        ldx     #0
        jsr     _platform_msleep
        jsr     __OPPONENT_START__+OPPONENT::WIN_POINT_SND
        ; Return with carry set to inform main
        sec
        inc     their_score
        rts
.endproc

.proc update_screen_for_crash
        jsr     _crash_lines_scale
        jsr     _transform_puck_coords
        jsr     _draw_screen

        jmp     _draw_crash_lines
.endproc

; A: standard 8-bit Y
.proc _init_precise_x
        ldx     #0
        clc
        asl
        sta     puck_precise_x
        bcc     :+
        inx
:       stx     puck_precise_x+1
        rts
.endproc
; A: standard 8-bit Y
.proc _init_precise_y
        ldx     #0
        clc
        asl
        sta     puck_precise_y
        bcc     :+
        inx
:       stx     puck_precise_y+1
        rts
.endproc

.bss
tmpx:            .res 1
tmpy:            .res 1

puck_right_x:    .res 1

puck_x:          .res 1
puck_precise_x:  .res 2
puck_dx:         .res 2

puck_y:          .res 1
puck_precise_y:  .res 2
puck_dy:         .res 2

my_pusher_x:     .res 1
my_pusher_y:     .res 1
their_pusher_x:  .res 1
their_pusher_y:  .res 1
their_pusher_dx: .res 1
their_pusher_dy: .res 1

puck_backup:     .res 10

player_caught:   .res 1
