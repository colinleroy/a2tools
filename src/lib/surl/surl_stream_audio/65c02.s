;
; Colin Leroy-Mira <colin@colino.net>, 2024
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.

; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; You should have received a copy of the GNU General Public License
; along with this program. If not, see <http://www.gnu.org/licenses/>.

.include "imports.inc"
.include "constants.inc"
.include "zp-variables.inc"
.include "cycle-wasters.inc"

.define STREAMER_65C02
; -----------------------------------------------------------------------------
; CPU-specific constant and macros
.macro ____SPKR_DUTY____4       ; Toggle speaker
        sta     SPKR            ; 4
.endmacro

.macro ____SPKR_DUTY____5 DC    ; Toggle speaker slower (DC ignored on 65c02)
          sta     (spkr_ptr)    ; 5
.endmacro

.macro STORE_TARGET_3           ; Useless here
          WASTE_3
.endmacro

.macro STORE_TARGET_4           ; Useless here
          WASTE_4
.endmacro

.macro JMP_NEXT_6
        jmp     (next,x)        ; 45    Done
.endmacro

        .code

; ------------------------------------------------------------------------
.align 256                      ; Make sure we don't cross page in the middle
_AUDIO_CODE_START = *           ; of duty cycle handlers.
_SAMPLES_BASE = *

; Ask proxy to send levels from 0-31, multiplied by 2. (no time for us to do it)
SAMPLE_OFFSET = 0
SAMPLE_MULT   = 2

.include "duty-cycles/0.s"

.include "vars.s"
.include "silence.s"
.include "../surl_stream_common/patch_addresses.s"
.include "../surl_stream_common/patch_audio_registers.s"
.include "patch_vu_meters.s"

; ------------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$100, error
.include "duty-cycles/1.s"
.include "duty-cycles/2.s"

.include "setup_pointers.s"

; ------------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$200, error
.include "duty-cycles/3.s"
.include "duty-cycles/4.s"

.include "vu_patches_b.s"
.include "status_patches.s"

; ------------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$300, error
.include "duty-cycles/5.s"
.include "duty-cycles/6.s"

.include "vu_patches_a.s"

; --------------------------------------
; 65c02-specific data
next:
                .word duty_cycle0
                .word duty_cycle1
                .word duty_cycle2
                .word duty_cycle3
                .word duty_cycle4
                .word duty_cycle5
                .word duty_cycle6
                .word duty_cycle7
                .word duty_cycle8
                .word duty_cycle9
                .word duty_cycle10
                .word duty_cycle11
                .word duty_cycle12
                .word duty_cycle13
                .word duty_cycle14
                .word duty_cycle15
                .word duty_cycle16
                .word duty_cycle17
                .word duty_cycle18
                .word duty_cycle19
                .word duty_cycle20
                .word duty_cycle21
                .word duty_cycle22
                .word duty_cycle23
                .word duty_cycle24
                .word duty_cycle25
                .word duty_cycle26
                .word duty_cycle27
                .word duty_cycle28
                .word duty_cycle29
                .word duty_cycle30
                .word duty_cycle31
                .word update_title
                .word break_out

; ------------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$400, error
.include "duty-cycles/7.s"
.include "duty-cycles/8.s"
.include "duty-cycles/9.s"
.include "duty-cycles/10.s"
.include "duty-cycles/11.s"

; ------------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$500, error
.include "duty-cycles/12.s"
.include "duty-cycles/13.s"
.include "duty-cycles/14.s"
.include "duty-cycles/15.s"
.include "duty-cycles/16.s"

; ------------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$600, error
.include "duty-cycles/17.s"
.include "duty-cycles/18.s"
.include "duty-cycles/19.s"
.include "duty-cycles/20.s"
.include "break_out.s"

; ------------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$700, error
.include "duty-cycles/21.s"
.include "duty-cycles/22.s"
.include "duty-cycles/23.s"
.include "duty-cycles/24.s"
.include "surl_stream_audio.s"

; ------------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$800, error
.include "duty-cycles/25.s"
.include "duty-cycles/26.s"
.include "duty-cycles/27.s"
.include "duty-cycles/28.s"
.include "kbd_send.s"

; ------------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$900, error
.include "duty-cycles/29.s"
.include "duty-cycles/30.s"
.include "duty-cycles/31.s"
.include "update_title.s"

; Make sure we didn't cross page in duty cycle handlers
.assert * < _SAMPLES_BASE+$A00, error

.include "data_patches.s"

; We don't really care about crossing pages now.

_AUDIO_CODE_SIZE = * - _AUDIO_CODE_START
