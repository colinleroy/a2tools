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

        .export   _draw_dashboard
        .export   _draw_level_end
        .export   _clear_hgr_screen, _wait_for_input

        .import   num_lives, num_rubber_bands, num_battery, cur_score
        .import   time_counter, frame_counter
        .import   _print_number
        .import   _quick_draw_mini_plane
        .import   _quick_draw_mini_score
        .import   _quick_draw_battery_reverted
        .import   _quick_draw_rubber_band_reverted

        .import   bcd_input

        .import   _play_ding, _sleep

        .import   _time_bonus_str
        .import   _your_score_str
        .import   _game_won_str
        .import   _game_lost_str

        .import   _hi_scores_screen
        .import   _cputsxy, _cutoa

        .import   _mouse_check_fire
        .import   pushax
        .import   _bzero

        .include  "apple2.inc"
        .include  "font.gen.inc"
        .include  "mini_plane.gen.inc"
        .include  "rubber_band_reverted.gen.inc"
        .include  "battery_reverted.gen.inc"

.segment "LOWCODE"

SCORE_ICON_X       = 37
SCORE_ICON_Y       = 1

MINI_PLANE_ICON_X  = 37
MINI_PLANE_ICON_Y  = 21

RUBBER_BAND_ICON_X = 37
RUBBER_BAND_ICON_Y = 41

BATTERY_ICON_X     = 37
BATTERY_ICON_Y     = 61

.proc _draw_dashboard
        lda     frame_counter     ; Draw dashboard on odd frames
        and     #01
        bne     :+
        rts

:       ; Lives
        ldx     #MINI_PLANE_ICON_X
        ldy     #MINI_PLANE_ICON_Y+mini_plane_HEIGHT+font_HEIGHT+1
        lda     #0
        sta     bcd_input+1
        lda     num_lives
        jsr     _print_number

        ldx     #MINI_PLANE_ICON_X
        ldy     #MINI_PLANE_ICON_Y
        jsr     _quick_draw_mini_plane

        ; Score
        ldx     #SCORE_ICON_X
        ldy     #SCORE_ICON_Y+rubber_band_reverted_HEIGHT+font_HEIGHT+1
        lda     cur_score+1
        sta     bcd_input+1
        lda     cur_score
        jsr     _print_number

        ldx     #SCORE_ICON_X
        ldy     #SCORE_ICON_Y
        jsr     _quick_draw_mini_score

        ; Rubber bands
        ldx     #RUBBER_BAND_ICON_X
        ldy     #RUBBER_BAND_ICON_Y+rubber_band_reverted_HEIGHT+font_HEIGHT+1
        lda     #0
        sta     bcd_input+1
        lda     num_rubber_bands
        jsr     _print_number

        ldx     #RUBBER_BAND_ICON_X
        ldy     #RUBBER_BAND_ICON_Y
        jsr     _quick_draw_rubber_band_reverted

        ; Battery
        ldx     #BATTERY_ICON_X
        ldy     #BATTERY_ICON_Y+battery_reverted_HEIGHT+font_HEIGHT+1
        lda     #0
        sta     bcd_input+1
        lda     num_battery
        jsr     _print_number

        ldx     #BATTERY_ICON_X
        ldy     #BATTERY_ICON_Y
        jmp     _quick_draw_battery_reverted
.endproc

; Enter with carry set to display game won message
.proc _draw_level_end
        lda     #$00
        sta     game_won

        bcc     :+
        inc     game_won
        inc     time_counter      ; It will be $FF there.
:       sta     displayed

print_time_bonus:
        ; Time bonus
        ldx     #6
        lda     #8
        jsr     pushax
        lda     #<_time_bonus_str
        ldx     #>_time_bonus_str
        jsr     _cputsxy

        lda     time_counter
        ldx     #0
        jsr     _cutoa

        dec     time_counter      ; Decrement here so we can print it at 0...

print_score:
        ; Score
        ldx     #6
        lda     #9
        jsr     pushax
        lda     #<_your_score_str
        ldx     #>_your_score_str
        jsr     _cputsxy

        lda     cur_score
        ldx     cur_score+1
        jsr     _cutoa

        lda     game_won
        bne     print_level     ; We don't want to recount bonus if we won

        lda     displayed
        bne     :+
        lda     #1              ; Initial delay before starting to count
        ldx     #0
        jsr     _sleep
        inc     displayed

:       lda     time_counter      ; ...And still cash time bonus
        bmi     print_level
        inc     cur_score
        bne     :+
        inc     cur_score+1
:       jsr     _play_ding
        jmp     print_time_bonus

print_level:
        lda     num_lives         ; Does the player still have lives?
        beq     print_game_over   ; No, so they lost. hah.

        lda     game_won          ; Did they win the game?
        beq     print_done

        ldx     #6                ; Yes, display game won message
        lda     #17
        jsr     pushax
        lda     #<_game_won_str
        ldx     #>_game_won_str
        jsr     _cputsxy

        jsr     _wait_for_input
        jsr     _hi_scores_screen
        jmp     out

print_game_over:

        ldx     #6
        lda     #14
        jsr     pushax
        lda     #<_game_lost_str
        ldx     #>_game_lost_str
        jsr     _cputsxy
        jsr     _wait_for_input

print_done:
        ; If game over, send player to scores screen
        lda     num_lives
        bne     out

        jsr     _hi_scores_screen
out:
        rts
.endproc

.proc _wait_for_input
        bit     KBDSTRB
:       lda     KBD               ; Wait for key or click
        bmi     kbd_in
        jsr     _mouse_check_fire
        bcc     :-
        lda     #$00
        rts
kbd_in:
        bit     KBDSTRB
        rts
.endproc

.proc _clear_hgr_screen
        lda     #<$2000
        ldx     #>$2000
        jsr     pushax

        lda     #<$2000
        ldx     #>$2000
        jmp     _bzero
.endproc

        .bss

displayed:        .res 1
game_won:         .res 1
