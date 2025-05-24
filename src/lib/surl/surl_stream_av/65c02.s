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
; received byte being the next duty cycle's address, as an index in the 'next' array.
;
; Duty cycle functions have to be page-aligned so their cycle count is predictible.
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

; Warning about the alignment of the 32 duty cycles: as we read the next
; sample without verifying the ACIA's status register, we may read an
; incomplete byte, while it is being landed in the data register. They are read
; LSB to MSB, so, we have to align the duty cycles in a way that even when an
; incomplete read happens, we do not load the 'next' array out of bounds.
; This is why, here, we have the duty cycles range from 0 to 31:
; $00 = 00000000
; $1F = 00011111

.include "imports.inc"
.include "constants.inc"
.include "zp-variables.inc"
.include "cycle-wasters.inc"

.define STREAMER_65C02
; -----------------------------------------------------------------------------
; CPU-specific constant and macros

; Ask proxy to send levels from 0-31, multiplied by 2. (no time for us to do it)
SAMPLE_OFFSET     = 0
SAMPLE_MULT       = 2

.macro ____SPKR_DUTY____4       ; Toggle speaker
        sta     SPKR            ; 4
.endmacro

.macro ____SPKR_DUTY____5 DC    ; Toggle speaker slower (DC ignored on 65c02)
        sta     (spkr_ptr)      ; 5
.endmacro

.macro JUMP_NEXT_DUTY
        jmp     (next,x)  
.endmacro

.macro PREPARE_VIDEO_7
        PREPARE_VIDEO_S3
        PREPARE_VIDEO_E4
.endmacro

.macro PREPARE_VIDEO_S3
        WASTE_3          ; 3
.endmacro
.macro PREPARE_VIDEO_E4
        tya                     ; 2      Get video byte in A
        cpy     #PAGE_TOGGLE    ; 5      Check for page toggle
.endmacro

.macro JUMP_NEXT_9
        WASTE_3                 ; 3
        JUMP_NEXT_DUTY          ; 9      jump to next duty cycle
.endmacro

        .bss

got_art:          .res 1

        .segment        "CODE"

; The AVCODE segment location is non-important (apart of course it must not be
; in $2000-$6000), but it is important that duty cycles don't cross pages, for
; cycle counting.
.align $100
_SAMPLES_BASE = *
; -----------------------------------------------------------------
.include "duty-cycles/0.s"
; Put the page0 array at the correct place. It must be at $XX40.
.include "page0-array.s"
.include "duty-cycles/1.s"

.align $100
.assert * = _SAMPLES_BASE + $100, error
.include "duty-cycles/2.s"

; Put the page1 array at the correct place. It must be at $XY40. (where XY = XX+1)
.include "page1-array.s"

.include "duty-cycles/3.s"
.include "duty-cycles/4.s"
.include "calc_text_bases.s"

.align $100
.assert * = _SAMPLES_BASE + $200, error
.include "duty-cycles/5.s"
.include "duty-cycles/6.s"
.include "duty-cycles/7.s"
.include "duty-cycles/8.s"
.include "duty-cycles/9.s"
.include "duty-cycles/10.s"

.align $100
.assert * = _SAMPLES_BASE + $300, error
.include "duty-cycles/11.s"
.include "duty-cycles/12.s"
.include "duty-cycles/13.s"
.include "duty-cycles/14.s"
.include "duty-cycles/17.s"

.align $100
.assert * = _SAMPLES_BASE + $400, error
.include "duty-cycles/15.s"
.include "duty-cycles/16.s"
.include "duty-cycles/18.s"
.include "calc_bases.s"

.align $100
.assert * = _SAMPLES_BASE + $500, error
.include "duty-cycles/19.s"
.include "duty-cycles/20.s"
.include "duty-cycles/21.s"
.include "duty-cycles/22.s"
.include "duty-cycles/23.s"

.align $100
.assert * = _SAMPLES_BASE + $600, error
.include "duty-cycles/24.s"
.include "duty-cycles/25.s"
.include "duty-cycles/26.s"
.include "duty-cycles/27.s"
.include "duty-cycles/28.s"

.align $100
.assert * = _SAMPLES_BASE + $700, error
.include "duty-cycles/29.s"
.include "duty-cycles/30.s"
.include "duty-cycles/31.s"
.include "cycle-wasters.s"
.include "video-handler.s"

; Check we didn't cross page in the middle of video_sub
.assert * < _SAMPLES_BASE + $800, error

; The rest of the functions don't need to be aligned.

.include "break_out.s"
.include "surl_start_stream_av.s"
.include "../surl_stream_common/patch_addresses.s"
.include "../surl_stream_common/patch_audio_registers.s"
.include "patch_video_registers.s"

.include "video-status-data.inc"
.include "video-data-data.inc"
.include "audio-data.inc"

; Setup functions
.include "setup.s"
.include "strings-a.inc"
.include "strings-b.inc"
.include "update_load_progress.s"
.include "surl_stream_av_send_request.s"
.include "surl_stream_av_setup_ui.s"
.include "surl_stream_av_get_art.s"
.include "surl_stream_av_prepare_start.s"
.include "surl_stream_av_handle_preload.s"
.include "surl_stream_av.s"

; -----------------------------------------------------------------------------
; CPU-specific data (duty cycles handlers table)
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
                .word break_out     ; No need to make sure we cover every
                                    ; number from to 32 to 63, as serial
                                    ; transfers bytes LSB to MSB.
