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

        .export     _init_mouse, _read_mouse

        .export     _mouse_wait_vbl
        .export     _mouse_check_button
        .export     mouse_x, mouse_y
        .export     mouse_dx, mouse_dy
        .export     _flip_page_after_vbl

        .import     ostype, _freq

        .importzp   ptr1, tmp1, tmp2

        .interruptor    mouse_irq
        .destructor     _deinit_mouse, 17 ; Stop mouse before irq handler

        .include    "mouse-kernel.inc"
        .include    "apple2.inc"

SETMOUSE        = $12   ; Sets mouse mode
SERVEMOUSE      = $13   ; Services mouse interrupt
READMOUSE       = $14   ; Reads mouse position
CLEARMOUSE      = $15   ; Clears mouse position to 0 (for delta mode)
POSMOUSE        = $16   ; Sets mouse position to a user-defined pos
CLAMPMOUSE      = $17   ; Sets mouse bounds in a window
HOMEMOUSE       = $18   ; Sets mouse to upper-left corner of clamp win
INITMOUSE       = $19   ; Resets mouse clamps to default values and
                        ; sets mouse position to 0,0
TIMEDATA        = $1C   ; Set mouse interrupt rate

RINGBUF_SIZE    = 4

pos1_lo         := $0478
pos1_hi         := $0578
pos2_lo         := $04F8
pos2_hi         := $05F8
status          := $0778

        .segment "RODATA"

offsets:.byte   $05             ; Pascal 1.0 ID byte
        .byte   $07             ; Pascal 1.0 ID byte
        .byte   $0B             ; Pascal 1.1 generic signature byte
        .byte   $0C             ; Device signature byte

values: .byte   $38             ; Fixed
        .byte   $18             ; Fixed
        .byte   $01             ; Fixed
        .byte   $20             ; X-Y pointing device type 0

size    = * - values

.segment "CODE"

.proc _init_mouse
        lda     #<$C000
        sta     ptr1
        lda     #>$C000
        sta     ptr1+1

        ; Use softswitch VBL sync on IIe/IIe enh
        lda     ostype
        cmp     #$30
        bcc     next_slot
        cmp     #$40
        bcs     next_slot

        lda     #$4C
        sta     _mouse_wait_vbl

        ; Search for AppleMouse II firmware in slots 1 - 7
next_slot:
        inc     ptr1+1
        lda     ptr1+1
        cmp     #>$C800
        bcc     :+

        ; Mouse firmware not found
        sec
        rts

        ; Check Pascal 1.1 Firmware Protocol ID bytes
:       ldx     #size - 1
:       ldy     offsets,x
        lda     values,x
        cmp     (ptr1),y
        bne     next_slot
        dex
        bpl     :-

        ; Get and patch firmware address hibyte
        lda     ptr1+1
        sta     lookup+2
        sta     xparam+1
        sta     jump+2

        ; Disable interrupts now because setting the slot number makes
        ; the IRQ handler (maybe called due to some non-mouse IRQ) try
        ; calling the firmware which isn't correctly set up yet
        php
        sei

        ; Convert to and save slot number
        and     #$0F
        sta     slot

        ; Convert to and patch I/O register index
        asl
        asl
        asl
        asl
        sta     yparam+1

        ; Set VBL rate
        lda     _freq
        jsr     set_mouse_freq

        ; The AppleMouse II Card needs the ROM switched in
        ; to be able to detect an Apple //e and use RDVBL
        bit     $C082

        ; Reset mouse hardware
        ldx     #INITMOUSE
        jsr     firmware

        ; Switch in LC bank 2 for R/O
        bit     $C080

        ; Turn mouse on
        lda     #%00000001
        ldx     #SETMOUSE
        jsr     firmware

        ; Turn VBL interrupt on
        lda     #%00001001
        ldx     #SETMOUSE

        jmp     call_firmware_and_cli
.endproc

.proc _deinit_mouse
        lda     slot
        bne     :+
        rts                       ; No mouse installed

:       php
        sei

        ; Turn mouse off
        lda     #%00000000
        ldx     #SETMOUSE
        jmp     call_firmware_and_cli
.endproc

.segment "CODE"

.proc call_firmware_and_cli
        jsr     firmware

        ; Enable interrupts and return success
        plp
        clc
        rts
.endproc

firmware:
        ; Lookup and patch firmware address lobyte
lookup: ldy     $FF00,x         ; Patched at runtime
        sty     jump+1          ; Modify code below

        ; Apple II Mouse TechNote #1, Interrupt Environment with the Mouse:
        ; "Enter all mouse routines (...) with the X register set to $Cn
        ;  and Y register set to $n0, where n = the slot number."
xparam: ldx     #$FF            ; Patched at runtime
yparam: ldy     #$FF            ; Patched at runtime

jump:   jmp     $FFFF           ; Patched at runtime

;Apple II Mouse Technical Notes #2: Varying VBL Interrupt Rate
.proc set_mouse_freq
        ldx     ostype
        cpx     #$40
        bcs     out               ; Don't do that on IIc/gs

        ldx     #TIMEDATA
        jsr     firmware

out:    rts
.endproc

mouse_irq:
        lda     slot            ; Check for installed mouse
        beq     done            ; Nope! must return with carry clear.

        ; We have a mouse.

        ldx     #SERVEMOUSE     ; Check for mouse interrupt by calling firmware.
        jsr     firmware        ; It returns with carry clear if our IRQ (thanks!)

        lda     #$00
        sbc     #$00            ; Now A = $00 if carry set (not our interrupt) or $FF
                                ; if it is our interrupt.

        sta     irq_fired       ; Update flags
        sta     readable

        beq     done
        sta     $C030

done:   lsr                     ; Set carry accordingly to flag (or clear if coming from slot == 0)
        rts                     ; Return with carry set if our IRQ.

_read_mouse:
        lda     readable
        bne     :+
        rts

:       lda     #0
        sta     readable
        php
        sei
        ldx     #READMOUSE
        jsr     firmware

        ; Bit 7 6 5 4 3 2 1 0
            ; | | | | | | | |
            ; | | | | | | | \--- Previously, button 1 was up (0) or down (1)
            ; | | | | | | \----- Movement interrupt
            ; | | | | | \------- Button 0/1 interrupt
            ; | | | | \--------- VBL interrupt
            ; | | | \----------- Currently, button 1 is up (0) or down (1)
            ; | | \------------- X/Y moved since last READMOUSE
            ; | \--------------- Previously, button 0 was up (0) or down (1)
            ; \----------------- Currently, button 0 is up (0) or down (1)

        ; Extract button down values
        ldy     slot
        lda     status,y
        asl                     ;  C = Button 0 is currently down
        and     #%00100000      ; !Z = Button 1 is currently down

        ; Set button mask
        beq     :+
        lda     #MOUSE_BTN_RIGHT
:       bcc     :+
        ora     #MOUSE_BTN_LEFT

:       sta     mouse_b
        ; Get and set the new X position
        ; Don't bother with high byte, it's zero
        ; And update the ringbuffer
        ldx     ringbuf_idx

        clc
        lda     pos1_lo,y
        asl
x_double:
        nop
        sta     mouse_x

        sec                       ; Compute current delta
        sbc     prev_x
        sta     x_ring_buf,x      ; Add it in ringbuf

        clc                       ; Update mouse dx
        adc     mouse_dx
        sta     mouse_dx

        lda     mouse_x           ; Update prev_x
        sta     prev_x

        lda     pos2_lo,y
        asl
y_double:
        nop
        sta     mouse_y

        sec                       ; Compute current delta
        sbc     prev_y
        sta     y_ring_buf,x      ; Add it in ringbuf

        clc                       ; Update mouse dy
        adc     mouse_dy
        sta     mouse_dy

        lda     mouse_y           ; Update prev_y
        sta     prev_y

        ; Update the ringbuf index
        inx
        cpx     #RINGBUF_SIZE
        bne     :+
        ldx     #0
:       stx     ringbuf_idx

        ; And substract from delta
        lda     mouse_dx
        sec
        sbc     x_ring_buf,x
        sta     mouse_dx

        lda     mouse_dy
        sec
        sbc     y_ring_buf,x
        sta     mouse_dy

        plp                     ; Reenable interrupts
        rts


.proc waste_3250
        ldy     #4
:       ldx     #161                ; 812 cycles per Y
:       dex                         ; 2
        bne     :-                  ; 5
        dey
        bne     :--
waste_end:
        rts
.endproc

.proc _mouse_wait_vbl
        bit     _softswitch_wait_vbl ; Patched with JMP on IIe

        lda     #0              ; Skip a frame rather than flicker
        sta     vbl_ready
:       lda     vbl_ready
        beq     :-

pal_slow:
        lda     _freq           ; Shorten window at 50Hz to catch
        ; cmp     #TV_NTSC      ; sync bugs on my own hardware
        bne     waste_3250      ; According to testing, we have
        rts                     ; ~1600 extra cycles before it starts
                                ; flickering on real hardware.
.endproc

; IIe only
.proc _softswitch_wait_vbl
        bit     $C019               ; Softswitch VBL
        bpl     _softswitch_wait_vbl; Wait for bit 7 on (VBL ended)
:       bit     $C019               ; Softswitch VBL
        bmi     :-                  ; Wait for bit 7 off (VBL started)
        jmp     _mouse_wait_vbl::pal_slow
.endproc

; Returns 0 if mouse button not pressed
.proc _mouse_check_button
        jsr     _read_mouse
        lda     mouse_b
        rts
.endproc


waste_65:                      ; 6
        nop                    ; 2
        ldy       #10          ; 2
:       dey                    ; 5*(Y-1) + 4
        bne       :-           ;
        rts                    ; 6

_flip_page_after_vbl:
        lda       #$00
        sta       irq_fired

:       lda       irq_fired
        beq       :-

        ldx       #111        ; Skip ~120 lines, so we see the band on both PAL and NTSC
:       jsr       waste_65    ; 65
        dex                   ; 67
        bne       :-          ; 70

        bit       $C055       ; Toggle page 2
        ldx       #3
:       jsr       waste_65    ; Keep on page 2 for three lines
        dex                   ; 67
        bne       :-
        bit       $C054       ; Toggle page 1

        rts

.bss

irq_fired:       .res  1

.segment "DATA"
; Put every var in DATA because mouse IRQs will start
; before we're done with the ONCE segment, and will
; poke stuff in BSS which still contains ONCE code
mouse_b:         .res 1
vbl_ready:       .res 1
mouse_x:         .res 1
mouse_y:         .res 1
prev_x:          .res 1
prev_y:          .res 1
x_ring_buf:      .res RINGBUF_SIZE
y_ring_buf:      .res RINGBUF_SIZE
ringbuf_idx:     .res 1
mouse_dx:        .res 1
mouse_dy:        .res 1
playbox:         .res 4*2
slot:            .res 1
readable:        .res 1
