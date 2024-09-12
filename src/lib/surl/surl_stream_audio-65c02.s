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

        .export         _surl_stream_audio
        .export         _SAMPLES_BASE

        .importzp       _zp6, _zp8, _zp10, _zp12, tmp1, tmp2, ptr1, ptr2, ptr3

        .import         _serial_putc_direct
        .import         _simple_serial_set_irq
        .import         _simple_serial_flush
        .import         popa, VTABZ, putchardirect, uppercasemask

.ifdef IIGS
        .import         zilog_status_reg_r, zilog_data_reg_r
        .import         _get_iigs_speed, _set_iigs_speed
        .include        "accelerator.inc"
.else
        .import         acia_status_reg_r, acia_data_reg_r
.endif

        .include        "apple2.inc"

MAX_LEVEL         = 31

.ifdef IIGS
serial_status_reg = zilog_status_reg_r
serial_data_reg   = zilog_data_reg_r
HAS_BYTE          = $01
.else
serial_status_reg = acia_status_reg_r
serial_data_reg   = acia_data_reg_r
HAS_BYTE          = $08
.endif

INV_SPC      := ' '
SPC          := ' '|$80
SPKR         := $C030

ser_data     := $FFFF           ; Placeholders for legibility, going to be patched
ser_status   := $FFFF
txt_level    := $FFFF

spkr_ptr      = _zp6
dummy_zp      = tmp1
title_addr    = ptr3

; ---------------------------------------------------------
;
; Macros to ease cycle counting

.macro WASTE_2                  ; Cycles wasters
        nop
.endmacro

.macro WASTE_3
        sta     dummy_zp
.endmacro

.macro WASTE_4
        WASTE_2
        WASTE_2
.endmacro

.macro WASTE_5
      WASTE_2
      WASTE_3
.endmacro

.macro WASTE_6
        WASTE_2
        WASTE_4
.endmacro

.macro WASTE_7
        WASTE_2
        WASTE_5
.endmacro

.macro WASTE_8
        WASTE_4
        WASTE_4
.endmacro

.macro WASTE_9
        WASTE_4
        WASTE_5
.endmacro

.macro WASTE_10
        WASTE_4
        WASTE_6
.endmacro

.macro WASTE_11
        WASTE_6
        WASTE_5
.endmacro

.macro WASTE_12
        jsr     waste
.endmacro
waste:          rts

.macro ____SPKR_DUTY____4       ; Toggle speaker
        sta     SPKR            ; 4
.endmacro

.macro ____SPKR_DUTY____5       ; Toggle speaker slower (but without phantom-read)
          sta     (spkr_ptr)      ; 5
.endmacro

.macro KBD_LOAD_7               ; Check keyboard and jsr if key pressed (trashes A)
        lda     KBD             ; 4
        bpl     :+              ; 7
        jsr     kbd_send
:
.endmacro

        .code

; ------------------------------------------------------------------------
.align 256                      ; Make sure we don't cross page in the middle
_SAMPLES_BASE = *               ; of duty cycle handlers.

duty_cycle0:                    ; Max negative level, 8 cycles
        ____SPKR_DUTY____4      ; 4     Toggle speaker
        ____SPKR_DUTY____4      ; 8    Toggle speaker
        lda     #INV_SPC        ; 10    Set VU meter
v0a:    sta     txt_level       ; 14
        
s0:     lda     ser_status      ; 18    Check serial
        and     #HAS_BYTE       ; 20

        beq     :+              ; 22/23

d0:     ldx     ser_data        ; 26    Load serial

        lda     #SPC            ; 28    Unset VU meter
        WASTE_7                 ; 35

v0b:    sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_12                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle0     ;    45

; --------------------------------------
; Vars (in CODE to save a very little bit of space)
prevspd:   .res 1
vumeter_base: .res 2

; --------------------------------------
kbd_send:
        and     #$7F            ; Clear high bit
        bit     KBDSTRB         ; Clear keyboard strobe
        cmp     #' '
        beq     pause
        jmp     _serial_putc_direct
pause:
        jsr     _serial_putc_direct
:       lda     KBD
        bpl     :-
        and     #$7F            ; Clear high bit
        bit     KBDSTRB         ; Clear keyboard strobe
        jmp     _serial_putc_direct

; --------------------------------------
silence:
ssil:   lda     ser_status
        and     #HAS_BYTE
        beq     silence
dsil:   ldx     ser_data
start_duty:
        jmp     (next,x)

.ifdef IIGS
slow_iigs:
        ; Slow down IIgs
        jsr     _get_iigs_speed
        sta     prevspd
        lda     #SPEED_SLOW
        jmp    _set_iigs_speed
.endif

; --------------------------------------
patch_addresses:                ; Patch all registers in ptr1 array with A
        ldy     #$00            ; Start at beginning
        sta     tmp1            ; Save value
next_addr:
        clc
        lda     (ptr1),y
        adc     #1              ; Add one to patch after label
        sta     ptr2
        iny
        lda     (ptr1),y
        beq     done            ; If high byte is 0, we're done
        adc     #0
        sta     ptr2+1
        iny

        phy
        ldy     #$00

        lda     tmp1            ; Patch low byte
        sta     (ptr2),y

        iny                     ; Patch high byte with base
        txa
        sta     (ptr2),y

        ply                     ; Y > 0 there

        bne     next_addr
done:
        rts

; --------------------------------------
patch_serial_registers:
        lda     #<(status_patches)
        sta     ptr1
        lda     #>(status_patches)
        sta     ptr1+1
        lda     serial_status_reg+1
        ldx     serial_status_reg+2
        jsr     patch_addresses

        lda     #<(data_patches)
        sta     ptr1
        lda     #>(data_patches)
        sta     ptr1+1
        lda     serial_data_reg+1
        ldx     serial_data_reg+2
        jsr     patch_addresses

        lda     #<(vu_patches_a)
        sta     ptr1
        lda     #>(vu_patches_a)
        sta     ptr1+1
        jsr     patch_vumeters

        lda     #<(vu_patches_b)
        sta     ptr1
        lda     #>(vu_patches_b)
        sta     ptr1+1
        ; jsr   patch_vumeters  ; Fallthrough 
        ; rts

; --------------------------------------
patch_vumeters:                 ; Patch all registers in ptr1 array with vumeter_base
        ldy     #$00            ; Start at beginning
        lda    vumeter_base
        sta    tmp1
next_vu:
        clc
        lda     (ptr1),y
        adc     #1              ; Add one to patch after label
        sta     ptr2
        iny
        lda     (ptr1),y
        beq     vu_done         ; If high byte is 0, we're done
        adc     #0
        sta     ptr2+1
        iny

        phy
        ldy     #$00

        lda     tmp1            ; Patch low byte
        sta     (ptr2),y

        iny                     ; Patch high byte with base
        lda     vumeter_base+1
        sta     (ptr2),y

        inc     tmp1            ; Increment vumeter address (next char)

        ply                     ; Y > 0 there

        bne     next_vu
vu_done:
        rts

; ------------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$100, error
duty_cycle1:
        ____SPKR_DUTY____4      ; 4     Toggle speaker
        ____SPKR_DUTY____5      ; 9    Toggle speaker
        lda     #INV_SPC        ; 11    Set VU meter
v1a:    sta     txt_level       ; 15
        
s1:     lda     ser_status      ; 19    Check serial
        and     #HAS_BYTE       ; 21

        beq     :+              ; 23/24

d1:     ldx     ser_data        ; 27    Load serial

        lda     #SPC            ; 29    Unset VU meter
        WASTE_6                 ; 35

v1b:    sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_11                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle1     ;    45

; --------------------------------------
setup_pointers:
        ; Setup vumeter X/Y
        sta     CV
        jsr     VTABZ
        jsr     popa
        sta     CH
        tay
        pha
        lda     #'('|$80
        sta     (BASL),y
        tya
        clc
        adc     #(MAX_LEVEL+2)
        sta     CH
        tay
        lda     #')'|$80
        sta     (BASL),y
        pla
        clc
        adc     #1
        adc     BASL
        sta     vumeter_base
        lda     BASH
        adc     #0
        sta     vumeter_base+1

        ; Setup title X/Y
        jsr     popa
        sta     CV
        jsr     VTABZ
        lda     BASL
        sta     title_addr
        lda     BASH
        sta     title_addr+1

        ; Setup numcols
        jsr     popa
        sta     numcols+1

        ; Setup pointer access to SPKR
        lda     #<(SPKR)
        sta     spkr_ptr
        lda     #>(SPKR)
        sta     spkr_ptr+1

        ; Setup a space for vumeter
        lda     #' '|$80
        sta     tmp2
        rts

; --------------------------------------
vu_patches_a:
                .word v0a
                .word v1a
                .word v2a
                .word v3a
                .word v4a
                .word v5a
                .word v6a
                .word v7a
                .word v8a
                .word v9a
                .word v10a
                .word v11a
                .word v12a
                .word v13a
                .word v14a
                .word v15a
                .word v16a
                .word v17a
                .word v18a
                .word v19a
                .word v20a
                .word v21a
                .word v22a
                .word v23a
                .word v24a
                .word v25a
                .word v26a
                .word v27a
                .word v28a
                .word v29a
                .word v30a
                .word v31a
                .word $0000

; --------------------------------------
duty_cycle2:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
        ____SPKR_DUTY____4      ; 10    Toggle speaker
v2a:    sta     txt_level       ; 14
        
s2:     lda     ser_status      ; 18    Check serial
        and     #HAS_BYTE       ; 20

        beq     :+              ; 22/23

d2:     ldx     ser_data        ; 26    Load serial

        lda     #SPC            ; 28    Unset VU meter
        WASTE_7                 ; 35

v2b:    sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_12                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle2     ;    45

; ------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$200, error
duty_cycle3:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
        ____SPKR_DUTY____5      ; 11    Toggle speaker
v3a:    sta     txt_level       ; 15
        
s3:     lda     ser_status      ; 19    Check serial
        and     #HAS_BYTE       ; 21

        beq     :+              ; 23/24

d3:     ldx     ser_data        ; 27    Load serial

        lda     #SPC            ; 29    Unset VU meter
        WASTE_6                 ; 35

v3b:    sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_11                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle3     ;    45

; --------------------------------------
vu_patches_b:
                .word v0b
                .word v1b
                .word v2b
                .word v3b
                .word v4b
                .word v5b
                .word v6b
                .word v7b
                .word v8b
                .word v9b
                .word v10b
                .word v11b
                .word v12b
                .word v13b
                .word v14b
                .word v15b
                .word v16b
                .word v17b
                .word v18b
                .word v19b
                .word v20b
                .word v21b
                .word v22b
                .word v23b
                .word v24b
                .word v25b
                .word v26b
                .word v27b
                .word v28b
                .word v29b
                .word v30b
                .word v31b
                .word $0000

; --------------------------------------
duty_cycle4:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
        WASTE_2                 ; 8
        ____SPKR_DUTY____4      ; 12    Toggle speaker
v4a:    sta     txt_level       ; 16
        
s4:     lda     ser_status      ; 20    Check serial
        and     #HAS_BYTE       ; 22

        beq     :+              ; 24/25

d4:     ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        WASTE_5                 ; 35

v4b:    sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_10                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle4     ;    45

; --------------------------------------
status_patches:
                .word s0
                .word s1
                .word s2
                .word s3
                .word s4
                .word s5
                .word s6
                .word s7
                .word s8
                .word s9
                .word s10
                .word s11
                .word s12
                .word s13
                .word s14
                .word s15
                .word s16
                .word s17
                .word s18
                .word s19
                .word s20
                .word s21
                .word s22
                .word s23
                .word s24
                .word s25
                .word s26
                .word s27
                .word s28
                .word s29
                .word s30
                .word s31
                .word ssil
                .word st
                .word st2
                .word $0000

; ------------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$300, error
duty_cycle5:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
        WASTE_3                 ; 9
        ____SPKR_DUTY____4      ; 13    Toggle speaker
v5a:    sta     txt_level       ; 17
        
s5:     lda     ser_status      ; 21    Check serial
        and     #HAS_BYTE       ; 23

        beq     :+              ; 25/26

d5:     ldx     ser_data        ; 29    Load serial

        lda     #SPC            ; 31    Unset VU meter
        WASTE_4                 ; 35

v5b:    sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_9                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle5     ;    45


; --------------------------------------
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

; --------------------------------------
data_patches:
                .word d0
                .word d1
                .word d2
                .word d3
                .word d4
                .word d5
                .word d6
                .word d7
                .word d8
                .word d9
                .word d10
                .word d11
                .word d12
                .word d13
                .word d14
                .word d15
                .word d16
                .word d17
                .word d18
                .word d19
                .word d20
                .word d21
                .word d22
                .word d23
                .word d24
                .word d25
                .word d26
                .word d27
                .word d28
                .word d29
                .word d30
                .word d31
                .word dsil
                .word dt
                .word dt2
                .word $0000

; --------------------------------------
duty_cycle6:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v6a:    sta     txt_level       ; 10
        ____SPKR_DUTY____4      ; 14    Toggle speaker
        
s6:     lda     ser_status      ; 18    Check serial
        and     #HAS_BYTE       ; 20

        beq     :+              ; 22/23

d6:     ldx     ser_data        ; 26    Load serial

        lda     #SPC            ; 28    Unset VU meter
        WASTE_7                 ; 35

v6b:    sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_12                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle6     ;    45

; ------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$400, error
duty_cycle7:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v7a:    sta     txt_level       ; 10
        ____SPKR_DUTY____5      ; 15    Toggle speaker
        
s7:     lda     ser_status      ; 19    Check serial
        and     #HAS_BYTE       ; 21

        beq     :+              ; 23/24

d7:     ldx     ser_data        ; 27    Load serial

        lda     #SPC            ; 29    Unset VU meter
        WASTE_6                 ; 35

v7b:    sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_11                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle7     ;    45

; --------------------------------------
duty_cycle8:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v8a:    sta     txt_level       ; 10
        WASTE_2                 ; 12
        ____SPKR_DUTY____4      ; 16    Toggle speaker
        
s8:     lda     ser_status      ; 20    Check serial
        and     #HAS_BYTE       ; 22

        beq     :+              ; 24/25

d8:     ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        WASTE_5                 ; 35

v8b:    sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_10                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle8     ;    45

; --------------------------------------
duty_cycle9:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v9a:    sta     txt_level       ; 10
        WASTE_3                 ; 13
        ____SPKR_DUTY____4      ; 17    Toggle speaker
        
s9:     lda     ser_status      ; 21    Check serial
        and     #HAS_BYTE       ; 23

        beq     :+              ; 25/26

d9:     ldx     ser_data        ; 29    Load serial

        lda     #SPC            ; 31    Unset VU meter
        WASTE_4                 ; 35

v9b:    sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_9                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle9     ;    45

; --------------------------------------
duty_cycle10:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v10a:   sta     txt_level       ; 10

s10:    lda     ser_status      ; 14    Check serial
        ____SPKR_DUTY____4      ; 18    Toggle speaker
        and     #HAS_BYTE       ; 20

        beq     :+              ; 22/23

d10:    ldx     ser_data        ; 26    Load serial

        lda     #SPC            ; 28    Unset VU meter
        WASTE_7                 ; 35

v10b:   sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_12                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle10    ;    45

; --------------------------------------
duty_cycle11:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v11a:   sta     txt_level       ; 10

s11:    lda     ser_status      ; 14    Check serial
        ____SPKR_DUTY____5      ; 19    Toggle speaker
        and     #HAS_BYTE       ; 21

        beq     :+              ; 23/24

d11:    ldx     ser_data        ; 27    Load serial

        lda     #SPC            ; 29    Unset VU meter
        WASTE_6                 ; 35

v11b:   sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_11                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle11    ;    45

; ------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$500, error
duty_cycle12:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v12a:   sta     txt_level       ; 10

s12:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        ____SPKR_DUTY____4      ; 20    Toggle speaker

        beq     :+              ; 22/23

d12:    ldx     ser_data        ; 26    Load serial

        lda     #SPC            ; 28    Unset VU meter
        WASTE_7                 ; 35

v12b:   sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_12                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle12    ;    45

; --------------------------------------
duty_cycle13:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v13a:   sta     txt_level       ; 10

s13:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        ____SPKR_DUTY____5      ; 21    Toggle speaker

        beq     :+              ; 23/24

d13:    ldx     ser_data        ; 27    Load serial

        lda     #SPC            ; 29    Unset VU meter
        WASTE_6                 ; 35

v13b:   sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_11                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle13    ;    45

; --------------------------------------
duty_cycle14:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v14a:   sta     txt_level       ; 10

s14:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        WASTE_2                 ; 18
        ____SPKR_DUTY____4      ; 22    Toggle speaker

        beq     :+              ; 24/25

d14:    ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        WASTE_5                 ; 35

v14b:   sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_10                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle14    ;    45

; --------------------------------------
duty_cycle15:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v15a:   sta     txt_level       ; 10

s15:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19

        ____SPKR_DUTY____5      ; 23    Toggle speaker
d15:    ldx     ser_data        ; 27    Load serial

        lda     #SPC            ; 29    Unset VU meter
        WASTE_6                 ; 35

v15b:   sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        ____SPKR_DUTY____4      ;    23 Toggle speaker
        WASTE_12                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle15    ;    45

; --------------------------------------
duty_cycle16:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v16a:   sta     txt_level       ; 10

s16:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19

        WASTE_2                 ; 20
        ____SPKR_DUTY____4      ; 24    Toggle speaker
d16:    ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        WASTE_5                 ; 35

v16b:   sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        ____SPKR_DUTY____5      ;    24 Toggle speaker
        WASTE_11                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle16    ;    45

; ------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$600, error
duty_cycle17:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v17a:   sta     txt_level       ; 10

s17:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19

        WASTE_3                 ; 21
        ____SPKR_DUTY____4      ; 25    Toggle speaker
d17:    ldx     ser_data        ; 29    Load serial

        lda     #SPC            ; 31    Unset VU meter
        WASTE_4                 ; 35

v17b:   sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_2                 ;    21
        ____SPKR_DUTY____4      ;    25 Toggle speaker
        WASTE_10                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle17    ;    45

; --------------------------------------
duty_cycle18:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v18a:   sta     txt_level       ; 10

s18:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19
d18:    ldx     ser_data        ; 22    Load serial

        ____SPKR_DUTY____4      ; 26    Toggle speaker
        lda     #SPC            ; 28    Unset VU meter
        WASTE_7                 ; 35

v18b:   sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_3                 ;    22
        ____SPKR_DUTY____4      ;    26 Toggle speaker
        WASTE_9                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle18    ;    45

; --------------------------------------
duty_cycle19:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v19a:   sta     txt_level       ; 10

s19:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19
d19:    ldx     ser_data        ; 22    Load serial

        ____SPKR_DUTY____5      ; 27    Toggle speaker
        lda     #SPC            ; 29    Unset VU meter
        WASTE_6                 ; 35

v19b:   sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_4                 ;    23
        ____SPKR_DUTY____4      ;    27 Toggle speaker
        WASTE_8                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle19    ;    45

; --------------------------------------
duty_cycle20:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v20a:   sta     txt_level       ; 10

s20:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19
d20:    ldx     ser_data        ; 22    Load serial

        WASTE_2                 ; 24
        ____SPKR_DUTY____4      ; 28    Toggle speaker
        lda     #SPC            ; 30    Unset VU meter
        WASTE_5                 ; 35

v20b:   sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_5                 ;    24
        ____SPKR_DUTY____4      ;    28 Toggle speaker
        WASTE_7                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle20    ;    45

; ------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$700, error
duty_cycle21:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v21a:   sta     txt_level       ; 10

s21:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19
d21:    ldx     ser_data        ; 22    Load serial

        WASTE_3                 ; 25
        ____SPKR_DUTY____4      ; 29    Toggle speaker
        lda     #SPC            ; 31    Unset VU meter
        WASTE_4                 ; 35

v21b:   sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_6                 ;    25
        ____SPKR_DUTY____4      ;    29 Toggle speaker
        WASTE_6                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle21    ;    45

; --------------------------------------
duty_cycle22:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v22a:   sta     txt_level       ; 10

s22:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19
d22:    ldx     ser_data        ; 22    Load serial

        lda     #SPC            ; 24    Unset VU meter
        WASTE_2                 ; 26

        ____SPKR_DUTY____4      ; 30    Toggle speaker
        WASTE_5                 ; 35
v22b:   sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_7                 ;    26
        ____SPKR_DUTY____4      ;    30 Toggle speaker
        WASTE_5                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle22    ;    45

; --------------------------------------
duty_cycle23:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v23a:   sta     txt_level       ; 10

s23:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19
d23:    ldx     ser_data        ; 22    Load serial

        lda     #SPC            ; 24    Unset VU meter
        WASTE_3                 ; 27

        ____SPKR_DUTY____4      ; 31    Toggle speaker
        WASTE_4                 ; 35
v23b:   sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_8                 ;    27
        ____SPKR_DUTY____4      ;    31 Toggle speaker
        WASTE_4                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle23    ;    45

; --------------------------------------
duty_cycle24:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v24a:   sta     txt_level       ; 10

s24:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19
d24:    ldx     ser_data        ; 22    Load serial

        lda     #SPC            ; 24    Unset VU meter
        WASTE_4                 ; 28

        ____SPKR_DUTY____4      ; 32    Toggle speaker
        WASTE_3                 ; 35
v24b:   sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_9                 ;    28
        ____SPKR_DUTY____4      ;    32 Toggle speaker
        WASTE_3                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle24    ;    45

; --------------------------------------
_surl_stream_audio:
        php
        sei                     ; Disable all interrupts
        pha

        lda     #$00            ; Disable serial interrupts
        jsr     _simple_serial_set_irq

        pla
        ; Setup pointers
        jsr     setup_pointers
        ; Patch serial registers
        jsr     patch_serial_registers

.ifdef IIGS
        jsr     slow_iigs
.endif

        lda     #$2F                    ; Ready
        jsr     _serial_putc_direct

        lda     numcols+1               ; Set numcols on proxy
        jsr     _serial_putc_direct
        lda     #0                      ; Send sample base
        jsr     _serial_putc_direct
        lda     #2                      ; Send sample multiplier
        jsr     _serial_putc_direct

        ; Start with silence
        jmp     silence

; ------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$800, error
duty_cycle25:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v25a:   sta     txt_level       ; 10

s25:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19
d25:    ldx     ser_data        ; 22    Load serial

        lda     #SPC            ; 24    Unset VU meter
        WASTE_5                 ; 29

        ____SPKR_DUTY____4      ; 33    Toggle speaker
        WASTE_2                 ; 35
v25b:   sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_10                ;    29
        ____SPKR_DUTY____4      ;    33 Toggle speaker
        WASTE_2                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle25    ;    45

; --------------------------------------
duty_cycle26:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v26a:   sta     txt_level       ; 10

        WASTE_2                 ; 12    Wait...

s26:    lda     ser_status      ; 16    Check serial
        and     #HAS_BYTE       ; 18
        beq     :+              ; 20/21
d26:    ldx     ser_data        ; 24    Load serial

        lda     #SPC            ; 26    Unset VU meter
v26b:   sta     txt_level       ; 30

        ____SPKR_DUTY____4      ; 34    Toggle speaker
        WASTE_5                 ; 39
        jmp     (next,x)        ; 45    Done
:
        KBD_LOAD_7              ;    28
        WASTE_2                 ;    30
        ____SPKR_DUTY____4      ;    34 Toggle speaker
        WASTE_8                 ;    42
        jmp     duty_cycle26    ;    45

; --------------------------------------
duty_cycle27:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v27a:   sta     txt_level       ; 10

        WASTE_3                 ; 13    Wait...

s27:    lda     ser_status      ; 17    Check serial
        and     #HAS_BYTE       ; 19
        beq     :+              ; 21/22
d27:    ldx     ser_data        ; 25    Load serial

        lda     #SPC            ; 27    Unset VU meter
        WASTE_4                 ; 31

        ____SPKR_DUTY____4      ; 35    Toggle speaker
v27b:   sta     txt_level       ; 39
        jmp     (next,x)        ; 45    Done
:
        WASTE_9                 ;    31
        ____SPKR_DUTY____4      ;    35 Toggle speaker
        KBD_LOAD_7              ;    42
        jmp     duty_cycle27    ;    45

; --------------------------------------
duty_cycle28:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v28a:   sta     txt_level       ; 10

        WASTE_4                 ; 14    Wait...

s28:    lda     ser_status      ; 18    Check serial
        and     #HAS_BYTE       ; 20
        beq     :+              ; 22/23
d28:    ldx     ser_data        ; 26    Load serial

        lda     #SPC            ; 28    Unset VU meter
v28b:   sta     txt_level       ; 32

        ____SPKR_DUTY____4      ; 36    Toggle speaker
        WASTE_3                 ; 39
        jmp     (next,x)        ; 45    Done
:
        KBD_LOAD_7              ;    30
        WASTE_2                 ;    32
        ____SPKR_DUTY____4      ;    36 Toggle speaker
        WASTE_6                 ;    42
        jmp     duty_cycle28    ;    45

; ------------------------------------------------------------------
.align 256
.assert * = _SAMPLES_BASE+$900, error
duty_cycle29:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v29a:   sta     txt_level       ; 10

        WASTE_5                 ; 15    Wait...

s29:    lda     ser_status      ; 19    Check serial
        and     #HAS_BYTE       ; 21
        beq     :+              ; 23/24
d29:    ldx     ser_data        ; 27    Load serial

        lda     #SPC            ; 29    Unset VU meter
v29b:   sta     txt_level       ; 33

        ____SPKR_DUTY____4      ; 37    Toggle speaker
        WASTE_2                 ; 39
        jmp     (next,x)        ; 45    Done
:
        KBD_LOAD_7              ;    31
        WASTE_2                 ;    33
        ____SPKR_DUTY____4      ;    37 Toggle speaker
        WASTE_5                 ;    42
        jmp     duty_cycle29    ;    45

; --------------------------------------
duty_cycle30:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

s30:    lda     ser_status      ; 8     Check serial
        and     #HAS_BYTE       ; 10
        beq     :+              ; 12/13
d30:    ldx     ser_data        ; 16    Load serial

        lda     next,x          ; 20    Update target
        sta     dest30+1        ; 24
        inx                     ; 26
        lda     next,x          ; 30

        WASTE_4                 ; 34
        ____SPKR_DUTY____4      ; 38    Toggle speaker
        sta     dest30+2        ; 42    Finish updating target

dest30: jmp     $0000           ; 45

:
        lda     #INV_SPC        ;    15 Set VU meter
v30a:   sta     txt_level       ;    19
        KBD_LOAD_7              ;    26
        WASTE_6                 ;    32
        lda     #SPC            ;    34
        ____SPKR_DUTY____4      ;    38 Toggle speaker
v30b:   sta     txt_level       ;    42 Unset VU meter
        jmp     duty_cycle30    ;    45

; --------------------------------------
duty_cycle31:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6     Set VU meter
v31a:   sta     txt_level       ; 10

        WASTE_7                 ; 17    Wait...

s31:    lda     ser_status      ; 21    Check serial
        and     #HAS_BYTE       ; 23
        beq     :+              ; 25/26
d31:    ldx     ser_data        ; 29    Load serial

        lda     #SPC            ; 31    Unset VU meter
v31b:   sta     txt_level       ; 35

        ____SPKR_DUTY____4      ; 39    Toggle speaker

        jmp     (next,x)        ; 45    Done
:
        KBD_LOAD_7              ;    33
        WASTE_2                 ;    35
        ____SPKR_DUTY____4      ;    39 Toggle speaker
        WASTE_3                 ;    42
        jmp     duty_cycle31    ;    45

; ------------------------------------------------------------------
; Make sure we didn't cross page in duty cycle handlers
.assert * < _SAMPLES_BASE+$A00, error

; We don't really care about crossing pages now.
; --------------------------------------
update_title:
        tya                     ; Backup Y
        pha

        lda     title_addr      ; gotoxy
        sta     BASL
        lda     title_addr+1
        sta     BASH
        lda     #0
        sta     CH

numcols:ldx     #0              ; Reset char counter

st:     lda     ser_status      ; 21    Check serial
        and     #HAS_BYTE       ; 23
        beq     st

                                ; 6502  65C02 (cycles since fetch, worst case,
                                ;              must be under 85 for serial access)
dt:     lda     ser_data        ; 4     4
        ora     #$80            ; 6     6

        .ifndef __APPLE2ENH__
        cmp     #$E0            ; 8            Should we uppercase?
        bcc     :+              ; 10/11
        and     uppercasemask   ; 13
:
        .endif

        pha                     ; 16    9      Local, stripped-down version of putchardirect
        .ifdef  __APPLE2ENH__   ;              to save 12 jsr+rts cycles
        lda     CH              ;       12
        bit     RD80VID         ;       16     In 80 column mode?
        bpl     put             ;       18/19  No, just go ahead
        lsr                     ;       20     Div by 2
        bcs     put             ;       22/23  Odd cols go in main memory
        bit     HISCR           ;       26     Assume SET80COL
put:    tay                     ;       28
        .else
        ldy     CH              ; 19
        .endif

        pla                     ; 22    31
        sta     (BASL),Y        ; 28    36

        .ifdef  __APPLE2ENH__
        bit     LOWSCR          ;       40     Doesn't hurt in 40 column mode
        .endif

        inc     CH              ; 33    45     Can't wrap line!

        dex                     ; 35    47
        bne     st              ; 37/38 49/50

        pla                     ; 40    52     Restore Y
        tay                     ; 42    54

st2:    lda     ser_status      ; 21    Check serial
        and     #HAS_BYTE       ; 23
        beq     st2
dt2:    ldx     ser_data
desttitle:
        jmp     (next,x)

; --------------------------------------
break_out:
.ifdef IIGS
        lda     prevspd
        jsr     _set_iigs_speed
.endif
        plp
        lda     #$01            ; Reenable IRQ and flush
        jsr     _simple_serial_set_irq
        jsr     _simple_serial_flush
        lda     #$2F            ; SURL_CLIENT_READY
        jmp     _serial_putc_direct

end_audio_streamer = *
