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

; General principles. we expect audio to be pushed on one serial port, video on
; the other. Execution flow  is controlled via the audio stream, with each
; received byte being the next duty cycle's address high byte.
;
; This means that duty cycle functions have to be page-aligned so their low byte
; is always 00 (and also for cycle counting).
; Functions that don't need to be aligned are stuffed between duty_cycle functions
; when possible, to waste less space.
;
; There are 32 different duty cycles (0-31), each moving the speaker by toggling
; it on and off at a certain cycles interval (from 8 to 39).
; The tasks of each duty cycle handler is:
; - Toggle the speaker on, wait a certain amount of cycles, toggle it off
; - Fetch the next byte of audio
; - Fetch the next byte of video
; - If there is a video byte, handle it
;
; It is possible to lose audio bytes without horrible effects (the sound quality
; would just be badder), but every video byte has to be fetched and handled,
; otherwise the video handler, being a state machine, will get messed up.
; Reading the same audio byte twice has no big drawback either, but reading the
; same video byte would, for the same reasons.
; In accordance to that, we spare ourselves the cycles required to verify the
; audio serial port's status register and unconditionnaly read the audio serial
; port's data register.
;
; Hence, each duty cycle does the following:
; - Load whatever is in the audio data register into jump destination
; - If there is a video byte,
;    - Load it,
;    - Finish driving speaker,
;    - Waste the required amount of cycles
;    - Jump to video handler
; - Otherwise,
;    - Load audio byte again,
;    - Waste the required amount of cycles (less than in the first case),
;    - Jump to the next duty cycle.
;
; Keyboard handling is cycle-expensive and can't be macroized properly, so
; reading the keyboard in cycle 16. This cycle, being in the middle, is,
; hopefully, called multiple hundreds of time per second. It's also the "silence"
; level so keyboard handling works on silent videos.
;
; The video handler is responsible for jumping directly to the next duty
; cycle once the video byte is handled.
;
; Almost every reference to serial registers is direct, so every duty cycle
; (in duty-cycles/*.s) is patched in multiple places. The patched instructions
; are labelled like ad0 (audio data cycle 0), and they need to be in *_patches
; arrays (in audio-data, video-status-data and video-data-data files) to be
; patched at start.
; Hardcoded placeholders have $C0xx where $xx is the register's address low
; byte for my setup, for reference.
; vs = video status = $99 (printer port in slot 1)
; vd = video data   = $98 (printer port in slot 1)
; as = audio status = $A9 (modem port in slot 2)
; ad = audio data   = $A8 (modem port in slot 2)
;
; 6502 limitation:
; Warning about the alignment of the 32 duty cycles: as we read the next
; sample without verifying the ACIA's status register, we may read an
; incomplete byte, while it is being landed in the data register.
;
; So, we have to align the duty cycles in a way that even when this happens,
; we do not jump to a place where we have no duty cycle. This is why, here,
; we have them from $6000 to $7F00:
; $60 = 01100000
; $7F = 01111111
;
; At worst, we'll play a wrong sample from time to time. Tests with duty
; cycles from $6400 to $8300 crashed into the monitor quite fast:
; $64 = 01100100
; $83 = 10000011
; Reading an incomplete byte there could result in reading 11100111, for
; example, but not only. We don't want that. Basically we want the highest
; possible bit set at 1 to be the same. 0x40-0x5F also fit. The rule is
; 0x?0 to 0x?0 + 0x1F.

.include "imports.inc"
.include "constants.inc"
.include "zp-variables.inc"
.include "cycle-wasters.inc"
; -----------------------------------------------------------------

; -----------------------------------------------------------------------------
; CPU-specific constant and macros

next       = _zp10            ; word - next cycle

.macro ____SPKR_DUTY____4       ; Toggle speaker
        sta     SPKR            ; 4
.endmacro

.macro ____SPKR_DUTY____5 DC    ; Toggle speaker slower (but without phantom-read)
          sta     SPKR-(SAMPLE_OFFSET+DC),x     ; 5
.endmacro

.macro JUMP_NEXT_DUTY
        jmp     (next)
.endmacro

.macro PREPARE_VIDEO_7
        PREPARE_VIDEO_S3
        PREPARE_VIDEO_E4
.endmacro

.macro PREPARE_VIDEO_S3
        stx     next+1          ; 3
.endmacro
.macro PREPARE_VIDEO_E4
        tya                     ; 2      Get video byte in A
        cpy     #PAGE_TOGGLE    ; 5      Check for page toggle
.endmacro

.macro JUMP_NEXT_9
        stx     next+1          ; 3
        JUMP_NEXT_DUTY          ; 9      jump to next duty cycle
.endmacro

        .bss
got_art:          .res 1

        .segment        "CODE"

; Align each duty cycle function
.align $100
_SAMPLES_BASE     = *
; Ask proxy to send levels from $?0 to $?0+$1F, not multiplied.
SAMPLE_OFFSET     = >_SAMPLES_BASE
SAMPLE_MULT       = 1
.assert SAMPLE_OFFSET .mod $20 = $0, error ; Cf $6000 to $7F00 comment at top of file
.include "duty-cycles/0.s"

.align $100
.assert * = _SAMPLES_BASE + $100, error
.include "duty-cycles/1.s"
; Add some setup function to fill in the gaps
.include "update_load_progress.s"
.include "surl_stream_av_send_request.s"
.include "surl_stream_av_setup_ui.s"
.include "surl_stream_av_get_art.s"

.align $100
.assert * = _SAMPLES_BASE + $200, error
.include "duty-cycles/2.s"
; Add some setup function to fill in the gaps
.include "surl_stream_av_prepare_start.s"
.include "surl_stream_av_handle_preload.s"
.include "surl_stream_av.s"

.align $100
.assert * = _SAMPLES_BASE + $300, error
.include "duty-cycles/3.s"
; Add some strings to fill in the gaps
.include "strings-a.inc"

.align $100
.assert * = _SAMPLES_BASE + $400, error
.include "duty-cycles/4.s"
; Add some strings to fill in the gaps
.include "strings-b40.inc"

.align $100
.assert * = _SAMPLES_BASE + $500, error
.include "duty-cycles/5.s"

.align $100
.assert * = _SAMPLES_BASE + $600, error
.include "duty-cycles/6.s"

.align $100
.assert * = _SAMPLES_BASE + $700, error
.include "duty-cycles/7.s"

.align $100
.assert * = _SAMPLES_BASE + $800, error
.include "duty-cycles/8.s"

.align $100
.assert * = _SAMPLES_BASE + $900, error
.include "duty-cycles/9.s"

.align $100
.assert * = _SAMPLES_BASE + $A00, error
.include "duty-cycles/10.s"

.align $100
.assert * = _SAMPLES_BASE + $B00, error
.include "duty-cycles/11.s"

.align $100
.assert * = _SAMPLES_BASE + $C00, error
.include "duty-cycles/12.s"

.align $100
.assert * = _SAMPLES_BASE + $D00, error
.include "duty-cycles/13.s"

.align $100
.assert * = _SAMPLES_BASE + $E00, error
.include "duty-cycles/14.s"

.align $100
.assert * = _SAMPLES_BASE + $F00, error
.include "duty-cycles/15.s"

.align $100
.assert * = _SAMPLES_BASE + $1000, error
.include "duty-cycles/16.s"

.align $100
.assert * = _SAMPLES_BASE + $1100, error
.include "duty-cycles/17.s"

.align $100
.assert * = _SAMPLES_BASE + $1200, error
.include "duty-cycles/18.s"

.align $100
.assert * = _SAMPLES_BASE + $1300, error
.include "duty-cycles/19.s"

.align $100
.assert * = _SAMPLES_BASE + $1400, error
.include "duty-cycles/20.s"

.align $100
.assert * = _SAMPLES_BASE + $1500, error
.include "duty-cycles/21.s"

.align $100
.assert * = _SAMPLES_BASE + $1600, error
.include "duty-cycles/22.s"

; Put the page0 array at the correct place. It must be at $XX40 with XX >= $54
.include "page0-array.s"

.align $100
.assert * = _SAMPLES_BASE + $1700, error
.include "duty-cycles/23.s"

; Put the page1 array at the correct place. It must be at $XY40. (where XY = XX+1)
.include "page1-array.s"

; Stuff code between duty cycles to optimize size
.include "video-handler.s"
.include "cycle-wasters.s"

.align $100
.assert * = _SAMPLES_BASE + $1800, error
.include "duty-cycles/24.s"

; Stuff data between duty cycles to optimize size
.include "video-status-data.inc"
.include "video-data-data.inc"

.align $100
.assert * = _SAMPLES_BASE + $1900, error
.include "duty-cycles/25.s"

; Stuff data between duty cycles to optimize size
.include "audio-data.inc"

.align $100
.assert * = _SAMPLES_BASE + $1A00, error
.include "duty-cycles/26.s"

; Stuff code between duty cycles to optimize size
.include "surl_start_stream_av.s"
.include "calc_bases.s"
.include "calc_text_bases.s"
.include "../surl_stream_common/patch_addresses.s"

.align $100
.assert * = _SAMPLES_BASE + $1B00, error
.include "duty-cycles/27.s"

; Stuff code between duty cycles to optimize size
.include "../surl_stream_common/patch_audio_registers.s"
.include "patch_video_registers.s"

.align $100
.assert * = _SAMPLES_BASE + $1C00, error
.include "duty-cycles/28.s"

.align $100
.assert * = _SAMPLES_BASE + $1D00, error
.include "duty-cycles/29.s"

; Stuff code between duty cycles to optimize size
.include "setup.s"

.align $100
.assert * = _SAMPLES_BASE + $1E00, error
.include "duty-cycles/30.s"

.align $100
.assert * = _SAMPLES_BASE + $1F00, error

.include "duty-cycles/31.s"

; Last function needing to be aligned (the exit function)
.align $100
.assert * = _SAMPLES_BASE + $2000, error
.include "break_out.s"

.assert * < _SAMPLES_BASE + $2100, error
.include "strings-b80.inc"

av_streamer_end = *
