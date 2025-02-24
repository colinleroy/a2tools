        .export     _init_mouse
        .export     mouse_b

        .export     reset_mouse
        .export     mouse_irq_ready

        .importzp   ptr1

        .import     _div7_table, _mod7_table

        .import     plane_data

        .interruptor    mouse_irq

        .include    "mouse-kernel.inc"
        .include    "apple2.inc"
        .include    "plane.gen.inc"
        .include    "sprite.inc"
        .include    "plane_coords.inc"

SETMOUSE        = $12   ; Sets mouse mode
SERVEMOUSE      = $13   ; Services mouse interrupt
READMOUSE       = $14   ; Reads mouse position
CLEARMOUSE      = $15   ; Clears mouse position to 0 (for delta mode)
POSMOUSE        = $16   ; Sets mouse position to a user-defined pos
CLAMPMOUSE      = $17   ; Sets mouse bounds in a window
HOMEMOUSE       = $18   ; Sets mouse to upper-left corner of clamp win
INITMOUSE       = $19   ; Resets mouse clamps to default values and
                        ; sets mouse position to 0,0

pos1_lo         := $0478
pos1_hi         := $0578
pos2_lo         := $04F8
pos2_hi         := $05F8
status          := $0778

CENTER_X_OFFSET  = (280-256)/2

        .bss

slot:    .res    1
mouse_b: .res    1

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
inibox: .word   plane_MIN_X
        .word   plane_MIN_Y
        .word   plane_MAX_X/2
        .word   plane_MAX_Y

        .data

mouse_prev_addr: .word $2000
mouse_prev_val: .byte $00
        .code

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

reset_mouse:
        ; Set initial mouse clamps
        lda     #<inibox
        ldx     #>inibox
        jsr     SETBOX

        ; Set initial mouse position
        ldx     slot
        lda     #>plane_MIN_X
        sta     pos1_hi,x
        lda     #<plane_MIN_X
        sta     pos1_lo,x
        asl
        sta     plane_x

        lda     #>plane_MIN_Y
        sta     pos2_hi,x
        lda     #<plane_MIN_Y
        sta     pos2_lo,x
        sta     plane_y

        ldx     #POSMOUSE
        jmp     firmware

_init_mouse:
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
common: jsr     firmware

        ; Enable interrupts and return success
        plp
        clc
        rts

SETBOX:
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
        bne     common          ; Branch always

mouse_irq:
        ; Check for installed mouse
        lda     slot
        beq     done

        ; Check for mouse interrupt
        ldx     #SERVEMOUSE
        jsr     firmware
        bcc     :+
        clc                     ; Interrupt not handled
done:   rts

:       php
        sei
        ldx     #READMOUSE
        jsr     firmware

        ; Get status
        ldy     slot
        lda     status,y
        tax                     ; Save status
        ; 
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
        ; Update mouse_b only on click and let the logic set it
        ; back to zero
        beq     :+
        lda     #MOUSE_BTN_RIGHT
:       bcc     :+
        ora     #MOUSE_BTN_LEFT
        sta     mouse_b
:

        ; Get and set the new X position
        ; Don't bother with high byte, it's zero
        lda     pos1_lo,y
        asl
        sta     plane_x

        ; Signal the main loop
        inc     mouse_irq_ready
        plp                     ; Reenable interrupts
        sec                     ; Interrupt handled
        rts

       .bss

mouse_irq_ready: .res 1
