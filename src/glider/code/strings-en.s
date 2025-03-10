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

        .export   _time_bonus_str
        .export   _your_score_str
        .export   _press_key_str
        .export   _no_level_str
        .export   _game_won_str
        .export   _game_lost_str
        .export   _your_name_str
        .export   _high_scores_str

        .data


_time_bonus_str:  .asciiz "           TIME BONUS: "
_your_score_str:  .asciiz "           YOUR SCORE: "
_press_key_str:   .asciiz "PRESS A KEY FOR LEVEL: "
_no_level_str:    .asciiz "     THERE IS NO LEVEL "
_game_won_str:    .asciiz "     YOU WON THE GAME!"
_game_lost_str:   .asciiz "           GAME OVER ! :-("

_your_name_str:   .asciiz "   ENTER YOUR NAME: "
_high_scores_str: .asciiz "         HIGH SCORES"
