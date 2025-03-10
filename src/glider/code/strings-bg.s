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

; Thanks to Milen Hristov (@circfruit@fosstodon.org)
_time_bonus_str:  .asciiz "           bonus wreme: "
_your_score_str:  .asciiz "                 to~ki: "
_press_key_str:   .asciiz "natisni klawi{ za niwo: "
_no_level_str:    .asciiz "              nqma niwo "
_game_won_str:    .asciiz "      prewyrtq igrata!"
_game_lost_str:   .asciiz "       igrata swyr{i ! :-("

_your_name_str:   .asciiz "   wywedi imeto si: "
_high_scores_str: .asciiz "          ranklista "
