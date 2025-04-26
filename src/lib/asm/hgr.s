;
; Copyright (C) 2022-2024 Colin Leroy-Mira <colin@colino.net>
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
;
        .export  _init_hgr
        .export  _init_text
        .export  _hgr_mixon
        .export  _hgr_mixoff
        .export  _hgr_init_done
        .export  _hgr_mix_is_on

        .import  ostype

        .constructor DetectLeChatMauveEve, 25

        .include "apple2.inc"
        .data

HR1_OFF         := $C0B2
HR1_ON          := $C0B3
HR2_OFF         := $C0B4
HR2_ON          := $C0B5
HR3_OFF         := $C0B6
HR3_ON          := $C0B7
TEXT16_OFF      := $C0B8
TEXT16_ON       := $C0B9

_hgr_init_done: .byte 0
_hgr_mix_is_on: .byte 0
has_eve:        .byte 0

        .segment "LOWCODE"

eve_bw:
        lda       has_eve
        beq       :+
        sta       DHIRESOFF   ; For EVE
        sta       HR1_OFF
        sta       HR2_ON
        sta       HR3_ON
:       rts

eve_color:
        lda       has_eve
        beq       :+
        sta       DHIRESOFF   ; For EVE
        sta       HR1_OFF
        sta       HR2_OFF
        sta       HR3_OFF
:       rts

; EVE doc, color modes,
; https://files.slack.com/files-pri/T1J8S1LGH-F08QEEH8DSL/download/le_chat_mauve_eve_-_manuel_de_reference.pdf?origin_team=T1J8S1LGH
; pages 117-118
_init_hgr:
        ldx       ostype
        cpx       #$20        ; No 80col card assumed in II+
        bcc       @do_init

        cmp       #$00
        beq       @color

@monochrome:
        jsr       eve_bw
        bne       @do_init

@color:
        jsr       eve_color

@do_init:
        lda       #$20
        sta       $E6         ; HGRPAGE

        ; Set HGR mode
        bit       CLR80COL
        bit       TXTCLR
        bit       MIXCLR
        bit       HIRES
        bit       LOWSCR
        bit       DHIRESOFF

        ldx       #1
        stx       _hgr_init_done
        dex
        stx       _hgr_mix_is_on
        rts

_init_text:
        bit       LOWSCR
        bit       TXTSET
        bit       LORES
        lda       #0
        sta       _hgr_init_done
        rts

_hgr_mixon:
        bit       $C053
        lda       #1
        sta       _hgr_mix_is_on
        rts

_hgr_mixoff:
        bit       $C052
        lda       #0
        sta       _hgr_mix_is_on
        rts

.segment "ONCE"

; From a2d
.proc DetectLeChatMauveEve
        ;; Based on IDENTIFICATION from the LCM Eve manual, pages 92-94

        kSentinelValue = $EE
        kModeValue = $0F
        .assert kSentinelValue <> kModeValue, error, "Values must differ"

        ;; Skip on IIc/IIc+ and IIgs
        bit     ostype
        bmi     done
        bvs     done

        ;; Detection routine
        php
        sei
        sta     SET80COL        ; let PAGE2 control banking
        sta     HISCR           ; access PAGE1X

        ldx     $400            ; save PAGE1X value in X
        lda     #kSentinelValue
        sta     $400            ; write to PAGE1X

        lda     #kModeValue
        sta     TEXT16_ON       ; TEXT16 on
        sta     LOWSCR          ; access PAGE1
        ldy     $400            ; save PAGE1 value in Y

        sta     $400            ; write to PAGE1
        sta     TEXT16_OFF      ; TEXT16 off
        sta     HISCR           ; access PAGE1X

        lda     #kSentinelValue
        eor     $400            ; did the value change?
        sta     result          ; if non-zero, Eve was shadowing

        stx     $400            ; restore PAGE1X from X
        sta     LOWSCR          ; access PAGE1
        sty     $400            ; restore PAGE1 from Y

        sta     CLR80COL        ; restore PAGE2 meaning
        plp

        result := *+1
done:   lda     #0              ; self-modified (but not always)
        sta     has_eve
        rts
.endproc
