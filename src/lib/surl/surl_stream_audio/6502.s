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

; -----------------------------------------------------------------------------
; CPU-specific constant and macros
.macro ____SPKR_DUTY____4       ; Toggle speaker
        sta     SPKR            ; 4
.endmacro

.macro ____SPKR_DUTY____5 DC    ; Toggle speaker slower (but without phantom-read)
          sta     SPKR-(SAMPLE_OFFSET+DC),x     ; 5
.endmacro

.macro STORE_TARGET_3           ; 3
          stx   zp_jmp+2
.endmacro

.macro STORE_TARGET_4           ; 4
          .byte $8E             ; stx abs
          .byte zp_jmp+2
          .byte $00
.endmacro

.macro JMP_NEXT_6               ; 6 because we jump to there
        jmp     zp_jmp
.endmacro

        .code

; ------------------------------------------------------------------------
.align 256                      ; Make sure we don't cross page in the middle
_SAMPLES_BASE = *               ; of duty cycle handlers.

; Ask proxy to send levels from this page, multiplied by 1. (no time for us to do it)
SAMPLE_OFFSET = >_SAMPLES_BASE
SAMPLE_MULT   = 1

.include "duty-cycles/0.s"
.include "vars.s"
.include "silence.s"
.include "../surl_stream_common/patch_addresses.s"
.include "../surl_stream_common/patch_audio_registers.s"
.include "patch_vu_meters.s"

.align 256
.assert * = _SAMPLES_BASE+$100, error
.include "duty-cycles/1.s"
.include "setup_pointers.s"
.include "vu_patches_a.s"
.include "kbd_send.s"

.align 256
.assert * = _SAMPLES_BASE+$200, error
.include "duty-cycles/2.s"

.align 256
.assert * = _SAMPLES_BASE+$300, error
.include "duty-cycles/3.s"
.include "vu_patches_b.s"

.align 256
.assert * = _SAMPLES_BASE+$400, error
.include "duty-cycles/4.s"
.include "status_patches.s"

.align 256
.assert * = _SAMPLES_BASE+$500, error
.include "duty-cycles/5.s"
.include "data_patches.s"

.align 256
.assert * = _SAMPLES_BASE+$600, error
.include "duty-cycles/6.s"

.align 256
.assert * = _SAMPLES_BASE+$700, error
.include "duty-cycles/7.s"

.align 256
.assert * = _SAMPLES_BASE+$800, error
.include "duty-cycles/8.s"

.align 256
.assert * = _SAMPLES_BASE+$900, error
.include "duty-cycles/9.s"

.align 256
.assert * = _SAMPLES_BASE+$A00, error
.include "duty-cycles/10.s"

.align 256
.assert * = _SAMPLES_BASE+$B00, error
.include "duty-cycles/11.s"

.align 256
.assert * = _SAMPLES_BASE+$C00, error
.include "duty-cycles/12.s"

.align 256
.assert * = _SAMPLES_BASE+$D00, error
.include "duty-cycles/13.s"

.align 256
.assert * = _SAMPLES_BASE+$E00, error
.include "duty-cycles/14.s"

.align 256
.assert * = _SAMPLES_BASE+$F00, error
.include "duty-cycles/15.s"

.align 256
.assert * = _SAMPLES_BASE+$1000, error
.include "duty-cycles/16.s"

.align 256
.assert * = _SAMPLES_BASE+$1100, error
.include "duty-cycles/17.s"

.align 256
.assert * = _SAMPLES_BASE+$1200, error
.include "duty-cycles/18.s"

.align 256
.assert * = _SAMPLES_BASE+$1300, error
.include "duty-cycles/19.s"

.align 256
.assert * = _SAMPLES_BASE+$1400, error
.include "duty-cycles/20.s"

.align 256
.assert * = _SAMPLES_BASE+$1500, error
.include "duty-cycles/21.s"

.align 256
.assert * = _SAMPLES_BASE+$1600, error
.include "duty-cycles/22.s"

.align 256
.assert * = _SAMPLES_BASE+$1700, error
.include "duty-cycles/23.s"

.align 256
.assert * = _SAMPLES_BASE+$1800, error
.include "duty-cycles/24.s"
.include "surl_stream_audio.s"

.align 256
.assert * = _SAMPLES_BASE+$1900, error
.include "duty-cycles/25.s"

.align 256
.assert * = _SAMPLES_BASE+$1A00, error
.include "duty-cycles/26.s"

.align 256
.assert * = _SAMPLES_BASE+$1B00, error
.include "duty-cycles/27.s"

.align 256
.assert * = _SAMPLES_BASE+$1C00, error
.include "duty-cycles/28.s"

.align 256
.assert * = _SAMPLES_BASE+$1D00, error
.include "duty-cycles/29.s"

.align 256
.assert * = _SAMPLES_BASE+$1E00, error
.include "duty-cycles/30.s"

.align 256
.assert * = _SAMPLES_BASE+$1F00, error
.include "duty-cycles/31.s"

.align 256
.assert * = _SAMPLES_BASE+$2000, error
.include "update_title.s"

.align 256
.assert * = _SAMPLES_BASE+$2100, error
.include "break_out.s"

end_audio_streamer = *
