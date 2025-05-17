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
        .export     _clear_screen_their_side, _draw_screen_their_side
        .export     _move_puck, _move_my_pusher, _move_their_pusher
        .export     _puck_check_my_hit, _puck_check_their_hit
        .export     _round_end

        .export     puck_x, puck_right_x, puck_y, puck_dx, puck_dy
        .export     _init_precise_x, _init_precise_y
        .export     my_pusher_x, my_pusher_y
        .export     their_pusher_x, their_pusher_y
        .export     their_pusher_dx, their_pusher_dy
        .export     their_currently_hitting, player_caught
        .export     their_hit_check_via_serial
        .export     _update_opponent

        .export     _puck_reinit_my_order, _puck_reinit_their_order
        .export     puck_in_front_of_me, puck_in_front_of_them
        .export     prev_puck_in_front_of_me, prev_puck_in_front_of_them
        .export     bounces
        .export     _transform_puck_coords

        .export     _guess_puck_x_at_y

        .import     _set_puck_position
        .import     _draw_sprite, _clear_sprite
        .import     _setup_sprite_pointer_for_clear
        .import     _setup_sprite_pointer_for_draw
        .import     _setup_eor_draw, _setup_eor_clear, _draw_eor, _clear_eor
        .import     mouse_x, mouse_y
        .import     mouse_dx, mouse_dy
        .import     _crash_lines_scale, _draw_crash_lines
        .import     my_score, their_score
        .import     _draw_score_update
        .import     _draw_opponent, _backup_table

        .import     _read_mouse

        .import     x_shift, x_factor, y_factor

        .import     umul8x8r16, _platform_msleep, _rand

        .import     _play_puck_hit
        .import     _play_puck
        .import     _play_crash

        .import     my_pushers_low, my_pushers_high, my_pushers_height, my_pushers_bytes, my_pushers_bpline
        .import     their_pushers_low, their_pushers_high, their_pushers_height, their_pushers_bytes, their_pushers_bpline
        .import     pucks_low, pucks_high, pucks_height, pucks_bytes, pucks_bpline

        .importzp   tmp1, ptr1

        .include    "helpers.inc"
        .include    "sprite.inc"
        .include    "puck_coords.inc"
        .include    "my_pusher_coords.inc"
        .include    "their_pusher_coords.inc"
        .include    "puck0.gen.inc"
        .include    "my_pusher0.gen.inc"
        .include    "constants.inc"
        .include    "opponent_file.inc"
        .include    "hgr_applesoft.inc"

.segment "LOWCODE"

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
        ; Get the correct puck sprite
        jsr     _puck_select

        ; Substract its height to draw it "on" the table
        lda     puck_real_gy
        sec
        sbc     puck_h
        sta     puck_gy

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
        jsr     my_pusher_select

        lda     my_pusher_real_gy
        sec
        sbc     my_pusher_h
        sta     my_pusher_gy

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
        jsr     their_pusher_select

        lda     their_pusher_real_gy
        sec
        sbc     their_pusher_h
        sta     their_pusher_gy

        lda     #<their_pusher_data
        ldx     #>their_pusher_data
        jsr     _setup_sprite_pointer_for_draw
        jmp     _draw_sprite
.endproc

; Render function when the puck is on the player's side
.proc render_screen_my_side
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
        ; Update previous stack flag for the next clear
        lda     puck_in_front_of_me
        sta     prev_puck_in_front_of_me
        bne     :+

        ; And draw in the correct order
        jsr     draw_puck
        jsr     draw_my_pusher
        rts

:       jsr     draw_my_pusher
        jsr     draw_puck
        rts
.endproc

.proc _update_opponent
        jsr     _clear_screen_their_side
        jsr     clear_my_pusher
        jsr     _draw_opponent
        jsr     _backup_table
        jsr     _draw_screen_their_side
        jmp     draw_my_pusher
.endproc

.proc _clear_screen_their_side
        lda     prev_puck_in_front_of_them
        beq     :+

        jsr     clear_puck
        jsr     clear_their_pusher
        rts

:       jsr     clear_their_pusher
        jsr     clear_puck
        rts
.endproc

.proc _draw_screen_their_side
        lda     puck_in_front_of_them
        sta     prev_puck_in_front_of_them
        bne     :+

        jsr     draw_puck
        jsr     draw_their_pusher
        rts

:       jsr     draw_their_pusher
        jsr     draw_puck
        rts
.endproc

; Render function when the puck is on the opponent's side
.proc render_screen_their_side
        ; Start with far side as it's higher on the screen,
        ; so we have less cycles to do it
        jsr     _clear_screen_their_side
        jsr     _draw_screen_their_side

        ; My side now
        jsr     clear_my_pusher
        jsr     draw_my_pusher
        rts
.endproc

; Draw screen, choosing which render function to use depending
; on the puck's side - knowing in which half of the table the puck
; is helps simplifying the draw order calculation
; ~ 12400 cycles
.proc _draw_screen
        lda     puck_y

        cmp     #MID_BOARD           ; Middle of HGR height

        bcs     :+
        jmp     render_screen_their_side
:       jmp     render_screen_my_side
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

; Offsets (theorical, not graphical) at which we change the sprite size
my_pusher_offsets:
        .byte   192
        .byte   183
        .byte   174
        .byte   165
NUM_MY_PUSHER_OFFSETS = *-my_pusher_offsets

.proc my_pusher_select
        lda     my_pusher_y
        ; Choose the smallest sprite
        ldx     #NUM_MY_PUSHER_OFFSETS-1
:       cmp     my_pusher_offsets,x
        ; Use it if the pusher is further away that the current offset
        bcc     out
        ; Else check the next bigger size
        dex
        ; Until we have the biggest sprite
        bne     :-

out:
        ; Update the sprite data fields (sprite and mask pointer,
        ; number of bytes and width)
        lda     my_pushers_low,x
        sta     my_pusher_data+SPRITE_DATA::SPRITE
        lda     my_pushers_high,x
        sta     my_pusher_data+SPRITE_DATA::SPRITE+1
        lda     my_pushers_bytes,x
        sta     my_pusher_data+SPRITE_DATA::BYTES
        lda     my_pushers_bpline,x
        sta     my_pusher_data+SPRITE_DATA::BYTES_WIDTH

        ; And store its height so we can offset the drawing correctly
        lda     my_pushers_height,x
        sta     my_pusher_h

        rts
.endproc

.proc _move_my_pusher
        ; Directly translate mouse coordinates to pusher coordinates
        jsr     _read_mouse
        ldy     mouse_y
        sty     my_pusher_y
        ldx     mouse_x
        stx     my_pusher_x

        ; Apply the "3D" transformation
        jsr     _transform_xy

        ; And store the graphical X directly in the sprite data,
        stx     my_pusher_gx

        ; And graphical Y in its dedicated variable that will be used
        ; to compute the Y offset in order to draw at the correct height
        ; (on the table rather than under)
        sty     my_pusher_real_gy
        rts
.endproc

their_pusher_offsets:
        .byte   50
        .byte   20
NUM_THEIR_PUSHER_OFFSETS = *-their_pusher_offsets

.proc their_pusher_select
        lda     their_pusher_y
        ldx     #NUM_THEIR_PUSHER_OFFSETS-1
:       cmp     their_pusher_offsets,x
        bcc     out
        dex
        bne     :-
out:
        lda     their_pushers_low,x
        sta     their_pusher_data+SPRITE_DATA::SPRITE
        lda     their_pushers_high,x
        sta     their_pusher_data+SPRITE_DATA::SPRITE+1
        lda     their_pushers_bytes,x
        sta     their_pusher_data+SPRITE_DATA::BYTES
        lda     their_pushers_bpline,x
        sta     their_pusher_data+SPRITE_DATA::BYTES_WIDTH

        lda     their_pushers_height,x
        sta     their_pusher_h

        rts
.endproc

.proc _move_their_pusher
        clc
        lda     their_pusher_x
        ADD_SIGNED_BOUNDS their_pusher_dx, #THEIR_PUSHER_MIN_X, #THEIR_PUSHER_MAX_X
        sta     their_pusher_x
        tax

        ; Now update Y
        clc
        lda     their_pusher_y
        ADD_SIGNED_BOUNDS their_pusher_dy, #THEIR_PUSHER_MIN_Y, #THEIR_PUSHER_MAX_Y
        sta     their_pusher_y

        tay
        jsr     _transform_xy
        stx     their_pusher_gx
        sty     their_pusher_real_gy
        rts
.endproc

puck_offsets:
        .byte   192
        .byte   175
        .byte   148
        .byte   121
        .byte   94
        .byte   67
        .byte   40
NUM_PUCK_OFFSETS = *-puck_offsets

.proc _puck_select
        lda     puck_y
        ldx     #NUM_PUCK_OFFSETS-1
:       cmp     puck_offsets,x
        bcc     out
        dex
        bne     :-
out:
        lda     pucks_low,x
        sta     puck_data+SPRITE_DATA::SPRITE
        lda     pucks_high,x
        sta     puck_data+SPRITE_DATA::SPRITE+1
        lda     pucks_bytes,x
        sta     puck_data+SPRITE_DATA::BYTES
        lda     pucks_bpline,x
        sta     puck_data+SPRITE_DATA::BYTES_WIDTH

        lda     pucks_height,x
        sta     puck_h

        rts
.endproc

; Update the sprite stack flag for our pusher / the puck
.proc _puck_reinit_my_order
        lda     puck_y
        cmp     my_pusher_y
        lda     #0
        rol
        sta     puck_in_front_of_me
        rts
.endproc

; Update the sprite stack flag for the opponent's pusher / the puck
.proc _puck_reinit_their_order
        lda     puck_y
        cmp     their_pusher_y
        lda     #0
        rol
        sta     puck_in_front_of_them
        rts
.endproc

; Make sure we don't go completely overboard with the puck's deltas
.proc bind_puck_speed
        lda     puck_dy
        BIND_SIGNED #ABS_MAX_DY
        sta     puck_dy

        lda     puck_dx
        BIND_SIGNED #ABS_MAX_DX
        sta     puck_dx
        clc                       ; Caller expects carry clear
        rts
.endproc

; Collision check between player pusher and puck
.proc _puck_check_my_hit
        ; Check if we already hit right before, we don't need to do it
        ; twice in a row
        lda     my_currently_hitting
        beq     check
        dec     my_currently_hitting

check:
        ; Y is checked by checking whether the stack order changed
        lda     puck_y
        cmp     my_pusher_y
        lda     #0
        rol
        cmp     puck_in_front_of_me
        bne     :+
        rts

        ; Stack order changed, store it
:       sta     puck_in_front_of_me

        ; Compare X to see if the pusher is in front of the puck, or missed it
        lda     my_pusher_x       ; Compare left,
        cmp     puck_right_x
        bcs     out_miss

        clc
        adc     #my_pusher0_WIDTH ; and add width for right bound
        bcs     :+                ; If adding our width overflowed, we're good:
        cmp     puck_x            ; the pusher is on the right and the puck too
        bcc     out_miss          ; as we checked left bound already

:       ; Prevent multiple hits
        lda     #1
        sta     my_currently_hitting

        ; Make sure puck doesn't go behind pusher so force its position one point
        ; in front of the pusher
        ldx     puck_x
        ldy     my_pusher_y
        dey
        jsr     _set_puck_position     ; Reinit the precise coords of the puck
        jsr     _puck_reinit_my_order  ; And set puck/pusher order while it goes away

        ; Update puck speed
        lda     puck_dx           ; Slow puck's existing deltaX
        cmp     #$80
        ror
        cmp     #$80
        ror
        beq     :+                ; Unless it would stop it
        sta     puck_dx
:       lda     mouse_dx          ; And add our own delta x
        clc
        adc     puck_dx
        sta     puck_dx

        ; Invert and slow puck delta-Y (if incoming!)
        lda     puck_dy
        bmi     :+                ; If the puck is already leaving, we just re-hit it
        NEG_A                     ; In which case we skip the slowing down and flag reset
        cmp     #$80
        ror
        sta     puck_dy

        lda     #1
        sta     player_caught     ; Used to inform opponents about the fact we caught the puck
                                  ; It's the opponent's responsability to zero the flag once they
                                  ; know

        ldy     #0                ; No sound slowing
        sty     their_currently_hitting ; Reset opponent hit (for Nerual who would patch dy multiple times)
        sty     bounces                 ; Reset X bounces counter (for Nerual)

        ; And play sound
        jsr     _play_puck_hit

        ; Add our delta-Y to the puck
:       lda     mouse_dy
        clc
        adc     puck_dy
        bmi     :+                ; Don't let DY still be positive or zero
        lda     #$FF              ; And make sure it goes away by one pixel
:       sta     puck_dy

        jmp     bind_puck_speed
out_miss:
        sec
        rts
.endproc

.proc _puck_check_their_hit
        ; Check if we already hit right before
        lda     their_currently_hitting
        beq     :+
        dec     their_currently_hitting
        jmp     _puck_reinit_their_order  ; Set puck/pusher order while it goes away

:       ; Check Y by checking whether the stack order changed
        lda     puck_y
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
        adc     #my_pusher0_WIDTH ; We use the standard width
        bcs     :+                ; If adding our width overflowed, we're good
        cmp     puck_x            ; the pusher is on the right and the puck too

        bcc     out_miss          ; Miss!

:       ; Prevent multiple hits
        lda     #15
        sta     their_currently_hitting
        ldy     #4
        jsr     _play_puck_hit

        ; Make sure puck doesn't go behind pusher
        ldx     puck_x
        ldy     their_pusher_y
        iny
        jsr     _set_puck_position; Reinit precise coordinates

        ; Update puck speed
        lda     puck_dx           ; Slow puck deltaX
        cmp     #$80
        ror
        cmp     #$80
        ror
        beq     :+                ; Unless it would make it zero
        sta     puck_dx
:       lda     their_pusher_dx   ; And add opponent's delta x
        cmp     #$80
        ror
        clc
        adc     puck_dx
        sta     puck_dx

        ; Invert and slow puck delta-Y
        lda     puck_dy
        beq     :+
        bpl     :+
        NEG_A
:       cmp     #$80
        ror
        cmp     #$80
        ror
        cmp     #$80
        ror
        sta     puck_dy
        lda     their_pusher_dy
        cmp     #$80
        ror
        bpl     :+
        NEG_A
:       clc
        adc     puck_dy
        cmp     #$01              ; Make sure the puck wouldn't stop
        bcs     store_dy
        lda     #$01
store_dy:
        sta     puck_dy
        clc
        jmp     bind_puck_speed

out_miss:
        sec                       ; Inform main that the opponent missed
        rts
.endproc

.proc play_revert_x
        ; Load the puck Y coordinate from the front
        lda     #PUCK_MAX_Y
        sec
        sbc     puck_y

        ; Divide it by 32 to get a 0-6 factor
        lsr
        lsr
        lsr
        lsr
        lsr

        ; And slow the sound sample accordingly
        tay
        jmp     _play_puck
.endproc

.proc _transform_puck_coords
        ldx     puck_x
        ldy     puck_y
        jsr     _transform_xy
        stx     puck_gx
        sty     puck_real_gy
        rts
.endproc

; Make sure variables are where we think, for arrival X guessing backup/restore
.assert puck_precise_x = puck_x + 1, error
.assert puck_dx = puck_precise_x + 2, error
.assert puck_y = puck_dx + 2, error
.assert puck_precise_y = puck_y + 1, error
.assert puck_dy = puck_precise_y + 2, error

; Compute the puck's X coordinate at desired Y (in A register).
; Returns with result in A
.proc _guess_puck_x_at_y
        sta     tmp1

        ; Backup the curent puck X/Y, precise X/Y, DX and DY variables
        ldy     #9
:       lda     puck_x,y
        sta     puck_backup,y
        dey
        bpl     :-

        ; Disable collision checks
        lda     #0
        sta     check_hits

        ; And simulate the puck move until we reach the desired Y
:       lda     puck_y
        cmp     tmp1
        beq     out
        bcc     out

        jsr     _move_puck
        jmp     :-

out:
        ; Re-enable the collision checks
        lda     #1
        sta     check_hits

        ; Get the result X
        ldx     puck_x

        ; Restore the current variables
        ldy     #9
:       lda     puck_backup,y
        sta     puck_x,y
        dey
        bpl     :-

        ; And return the value
        txa
        rts
.endproc

check_hits: .byte 1

; Bounce puck on the table sides
.proc revert_x
        lda     puck_dx
        NEG_A
        sta     puck_dx
        lda     check_hits    ; Are we simulating?
        beq     :+
        jsr     play_revert_x ; If so, don't play the sound
        inc     bounces       ; and don't increase the bounces counter
:
        ; And go back to moving puck (fallthrough)
.endproc

.proc _move_puck
        ; Extend puck_dx to 16bits for a more precise movement
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

        ; Check X bound and bounce on the table side if necessary
        cpx     #$FF
        beq     revert_x
        cpx     #$02
        bcs     revert_x
        cmp     #PUCK_MAX_X
        bcs     revert_x
        sta     puck_x            ; Update puck_x

        clc
        adc     #puck0_WIDTH      ; and puck_right_x while we're at it
        bcc     :+                ; Don't let it overflow.
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

        cmp     #$FF              ; Do not allow puck Y to go negative
        bne     check_y_bound
        lda     #$00
        sta     puck_precise_y
        sta     puck_precise_y+1

check_y_bound:
        tax                       ; Save high byte to X
        lsr                       ; Divide by two
        lda     puck_precise_y
        ror
        sta     puck_y

        ldy     check_hits        ; Are we simulating?
        beq     out

        ; Check opponent catch, their pusher might be at the table's border
        ; In this case the Y checking via stack order wouldn't have worked
        cpx     #$FF
        beq     check_their_late_catch
        cmp     #PUCK_MIN_Y
        bcc     check_their_late_catch

        ; Check our catch, our pusher might be at the table's border
        ; In this case the Y checking via stack order wouldn't have worked
        cmp     #PUCK_MAX_Y
        bcs     check_my_late_catch

        ; Set the graphical coords according to the theorical ones
        jsr     _transform_puck_coords

        clc
out:    rts

check_their_late_catch:
        ldx     puck_x
        ldy     #PUCK_MIN_Y
        jsr     _set_puck_position; Reinit precise coordinates

        jsr     _puck_check_their_hit
        bcc     update_y

        ; Serial hook - the oppponent missed, but we don't want
        ; to inform our game engine via carry as usual - otherwise
        ; we'll start resetting the point maybe before the other
        ; player send the "I missed" message. So just stop the puck
        ; and wait for them to tell us.
        lda     their_hit_check_via_serial
        beq     set_carry
        lda     #$00
        sta     puck_dx
        sta     puck_dy
        clc
        rts

set_carry:
        sec
        rts

check_my_late_catch:
        ldx     puck_x
        ldy     #PUCK_MAX_Y
        jsr     _set_puck_position; Reinit precise coordinates

        jsr     _puck_check_my_hit
        bcc     update_y

        sec
        rts
.endproc

.proc _round_end
        ; Stop their pusher
        lda     #0
        sta     their_pusher_dx
        sta     their_pusher_dy

        ; Clear their side to load their sprite cleanly
        jsr     _clear_screen_their_side

        ; Where is the puck?
        lda     puck_y
        cmp     #PUCK_MAX_Y
        beq     they_win

we_win:
        ; Put up their "lose" sprite
        jsr     __OPPONENT_START__+OPPONENT::LOSE_POINT

        ; Make sure puck_y didn't underflow
        ldx     puck_x
        ldy     #PUCK_MIN_Y
        jsr     _set_puck_position

        ; Their side's sprites will be redrawn by update_screen_for_crash
        ; so don't bother with it here

        ; Set crash lines parameters
        clc                       ; Little crash
        jsr     update_screen_for_crash

        ; Play crash sound, a bit slowed
        ldy     #2
        jsr     _play_crash

        ; Wait 200ms
        lda     #200
        ldx     #0
        jsr     _platform_msleep

        ; Play their lose sound (and/or animation)
        jsr     _rand
        cmp     #<(255*2/3)       ; 2/3 chances to play the sound
        bcs     :+
        jsr     __OPPONENT_START__+OPPONENT::LOSE_POINT_SND

:       ; Increment our score,
        inc     my_score

        ; And call the hand drawing with carry clear so it updates our line
        clc
        jsr     _draw_score_update

        ; Return with carry set to inform main that the round is over
        sec
        rts

they_win:
        jsr     __OPPONENT_START__+OPPONENT::WIN_POINT

        ; Set crash lines parameters
        sec                       ; Large crash, on our side
        jsr     update_screen_for_crash

        ; Play crash sound, a bit slowed
        ldy     #0
        jsr     _play_crash

        lda     #200
        ldx     #0
        jsr     _platform_msleep

        ; Play their win sound (and/or animation)
        jsr     _rand
        cmp     #<(255*2/3)       ; 2/3 chances to play the sound
        bcs     :+
        jsr     __OPPONENT_START__+OPPONENT::WIN_POINT_SND
:       inc     their_score

        ; And call the hand drawing with carry clear so it updates their line
        sec
        jsr     _draw_score_update

        ; Return with carry set to inform main that the round is over
        sec
        rts
.endproc

; Draw the crash lines on the screen
.proc update_screen_for_crash
        jsr     _crash_lines_scale
        jsr     _transform_puck_coords
        jsr     _draw_screen

        jmp     _draw_crash_lines
.endproc

; A: standard 8-bit X
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

my_pusher_real_gy:    .res 1
their_pusher_real_gy: .res 1
puck_real_gy:         .res 1
my_pusher_h:          .res 1
their_pusher_h:       .res 1
puck_h:               .res 1

puck_backup: .res 10

player_caught:              .res 1
puck_in_front_of_me:        .res 1
puck_in_front_of_them:      .res 1

prev_puck_in_front_of_me:   .res 1
prev_puck_in_front_of_them: .res 1

my_pusher_mid_x:            .res 1
my_currently_hitting:       .res 1
their_currently_hitting:    .res 1
bounces:                    .res 1

; For serial
their_hit_check_via_serial: .res 1
