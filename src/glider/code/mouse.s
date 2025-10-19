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
        .export     vbl_ready

        .import     ostype, _freq

        .export     _mouse_reset_ref_x
        .export     _mouse_update_ref_x
        .export     _mouse_wait_vbl
        .export     _mouse_check_fire

        .export     prev_x, ref_x, mouse_x    ; Shared with keyboard.s

        .importzp   ptr1

        .import     _check_battery_boost
        .import     _keyboard_reset_ref_x

        .interruptor    mouse_irq
        .destructor     _deinit_mouse, 17

        .include    "mouse-kernel.inc"
        .include    "apple2.inc"
        .include    "plane.gen.inc"
        .include    "sprite.inc"
        .include    "plane_coords.inc"
        .include    "constants.inc"

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

pos1_lo         := $0478
pos1_hi         := $0578
pos2_lo         := $04F8
pos2_hi         := $05F8
status          := $0778

        .rodata

offsets:.byte   $05             ; Pascal 1.0 ID byte
        .byte   $07             ; Pascal 1.0 ID byte
        .byte   $0B             ; Pascal 1.1 generic signature byte
        .byte   $0C             ; Device signature byte

values: .byte   $38             ; Fixed
        .byte   $18             ; Fixed
        .byte   $01             ; Fixed
        .byte   $20             ; X-Y pointing device type 0

size    = * - values

; Box to the part where our paddle can move
inibox: .word   2
        .word   0
        .word   255
        .word   255

        .data

mouse_prev_addr: .word $2000
mouse_prev_val: .byte $00

.segment "LOWCODE"

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

.proc _mouse_reset_ref_x
        jsr     _keyboard_reset_ref_x

        ; Set initial mouse position
        php
        sei

        ; Reset fire indicator
        lda     #$00
        sta     mouse_b

        ; Reset position
        ldx     slot
        lda     #$02
        sta     pos1_hi,x
        lda     #$02
        sta     pos1_lo,x

        lda     #>plane_MIN_Y
        sta     pos2_hi,x
        lda     #<plane_MIN_Y
        sta     pos2_lo,x

        ldx     #POSMOUSE
        jsr     firmware
        plp
        rts
.endproc

.proc _init_mouse
        lda     #<$C000
        sta     ptr1
        lda     #>$C000
        sta     ptr1+1

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
mouse_common:
        jsr     firmware

        ; Enable interrupts and return success
        plp
        clc
        rts
.endproc                          ; Not really endproc, but common needs to be out

.proc _deinit_mouse
        lda     slot
        bne     :+
        rts                       ; No mouse installed

:       php
        sei

        ; Turn mouse off
        lda     #%00000000
        ldx     #SETMOUSE
        bne     _init_mouse::mouse_common
.endproc

.proc mouse_setbox
        sta     ptr1
        stx     ptr1+1

        ; Set x clamps
        ldx     #$00
        ldy     #MOUSE_BOX::MINX
        jsr     :+

        ; Set y clamps
        ldx     #$01
        ldy     #MOUSE_BOX::MINY

        ; Apple II Mouse TechNote #1, Interrupt Environment with the Mouse:
        ; "Disable interrupts before placing position information in the
        ;  screen holes."
:       php
        sei

        ; Set low clamp
        lda     (ptr1),y
        sta     pos1_lo
        iny
        lda     (ptr1),y
        sta     pos1_hi

        ; Skip one word
        iny
        iny

        ; Set high clamp
        iny
        lda     (ptr1),y
        sta     pos2_lo
        iny
        lda     (ptr1),y
        sta     pos2_hi

        txa
        ldx     #CLAMPMOUSE
        bne     _init_mouse::mouse_common      ; Branch always
.endproc

.proc mouse_irq
        ; Check for installed mouse
        lda     slot
        beq     done

        ; Check for mouse interrupt
        ldx     #SERVEMOUSE
        jsr     firmware
        bcc     :+
        clc                       ; Interrupt not handled
done:   rts

:       sec
        ; Signal the main loop
        inc     vbl_ready
        ; And mouse_read
        inc     readable
        rts
.endproc

.proc _read_mouse
        lda     readable
        bne     :+
        rts

:       lda     #0
        sta     readable
        php
        sei
        ldx     #READMOUSE
        jsr     firmware

        ; Get status
        ldy     slot
        lda     status,y
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
        asl                     ;  C = Button 0 is currently down
        and     #%00100000      ; !Z = Button 1 is currently down

        ; Set button mask
        ; Update mouse_b only on click and let the game logic set it
        ; back to zero
        beq     :+
        lda     #MOUSE_BTN_RIGHT
:       bcc     :+
        ora     #MOUSE_BTN_LEFT
        sta     mouse_b

:       ; Get and set the new X position
        ; Don't bother with high byte, it's zero
        ldx     pos1_lo,y
        stx     mouse_x

        plp                     ; Reenable interrupts
        rts
.endproc

; Return with ref_x in A
.proc _mouse_update_ref_x
        jsr     _read_mouse
        lda     ref_x
        ldx     mouse_x
        cpx     prev_x
        beq     mouse_out_not_handled         ; Mouse did not move
        bcc     mouse_neg         ; Mouse moved left

mouse_pos:
        clc
        adc     #PLANE_VELOCITY
        bcs     mouse_out_not_handled
        sta     ref_x

        jsr     _check_battery_boost
        jmp     mouse_out_handled

mouse_neg:
        sec
        sbc     #PLANE_VELOCITY
        bcc     mouse_out_not_handled
        sta     ref_x

mouse_out_handled:
        stx     prev_x          ; Backup mouse X for next comparison
        lda     ref_x
        sec
        rts
mouse_out_not_handled:
        stx     prev_x          ; Backup mouse X for next comparison
        lda     ref_x
        clc
        rts
.endproc

.proc waste_3250
        ldy     #4
:       ldx     #161
:       dex                         ; 2
        bne     :-                  ; 5
        dey
        bne     :--
        rts
.endproc

.proc _mouse_wait_vbl
        lda     vbl_ready
        beq     _mouse_wait_vbl
        lda     #0
        sta     vbl_ready

        lda     _freq
        ; cmp     #TV_NTSC 
        ; bne     waste_3250
        rts
.endproc

.proc _mouse_check_fire
        lda     mouse_b
        beq     :+
        lda     #0
        sta     mouse_b
        sec
        rts
:       clc
        rts
.endproc

;Apple II Mouse Technical Notes #2: Varying VBL Interrupt Rate
.proc set_mouse_freq
        ldx     ostype
        cpx     #$40
        bcs     out               ; Don't do that on IIc/gs

        ldx     #TIMEDATA
        jsr     firmware

out:    rts
.endproc

       .bss

slot:            .res 1
mouse_b:         .res 1
ref_x:           .res 1
vbl_ready:       .res 1
prev_x:          .res 1
mouse_x:         .res 1
readable:        .res 1
