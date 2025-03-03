        .export     softswitch_wait_vbl
        .export     keyboard_reset_ref_x
        .export     keyboard_update_ref_x
        .export     keyboard_check_fire
        .export     keyboard_level_change
        .export     keyboard_calibrate_hz

        .import     prev_x, ref_x, mouse_x    ; Shared with mouse.s
        .import     _check_battery_boost
        .import     plane_data
        .import     vbl_ready, hz

        .include    "apple2.inc"
        .include    "sprite.inc"
        .include    "plane_coords.inc"
        .include    "constants.inc"

softswitch_wait_vbl:
        bit     $C019               ; Softswitch VBL
        bmi     softswitch_wait_vbl ; Wait for bit 7 off (VBL ended)
:       bit     $C019               ; Softswitch VBL
        bpl     :-                  ; Wait for bit 7 on (VBL started)
        rts

keyboard_reset_ref_x:
        lda     #PLANE_ORIG_X
        sta     ref_x
        sta     mouse_x
        sta     plane_x
        sta     prev_x

        lda     #PLANE_ORIG_Y
        sta     plane_y
        rts

keyboard_update_ref_x:
        lda     #$FF
        sta     keyboard_level_change

        lda     ref_x
        ldy     KBDSTRB
        cpy     #($08|$80)        ; CURS_LEFT
        beq     kbd_neg
        cpy     #($15|$80)        ; CURS_RIGHT
        beq     kbd_pos
        cpy     #($20|$80)       ; SPACE
        beq     kbd_fire
.ifdef UNKILLABLE
        cpy     #($0B|$80)       ; CURS_UP
        beq     kbd_up
        cpy     #($0A|$80)       ; CURS_DOWN
        beq     kbd_down
.endif
        bit     BUTN0
        bpl     kbd_out
        ; Open-Apple is down. Clear high bit, check it's 'g'
        tya
        and     #$7F
        cmp     #'g'
        bne     kbd_out

        ; It is, now get key and go to level
        bit     KBDSTRB
:       lda     KBD
        bpl     :-
        and     #$7F
        sec
        sbc     #'a'
        sta     keyboard_level_change
        jmp     kbd_out

.ifdef UNKILLABLE
kbd_up:
        dec     plane_y
        jmp     kbd_out
kbd_down:
        inc     plane_y
        jmp     kbd_out
.endif
kbd_pos:
        clc
        adc     #PLANE_VELOCITY
        bcs     kbd_out
        sta     ref_x

        jsr     _check_battery_boost
        jmp     kbd_out

kbd_neg:
        sec
        sbc     #PLANE_VELOCITY
        bcc     kbd_out
        sta     ref_x
        jmp     kbd_out

kbd_fire:
        lda     #1
        sta     kbd_should_fire

kbd_out:
        lda     ref_x
        rts

keyboard_check_fire:
        lda     kbd_should_fire
        beq     :+
        lda     #0
        sta     kbd_should_fire
        sec
        rts
:       clc
        rts

keyboard_calibrate_hz:
        ; Wait for bit 7 to be off (no VBL)
:       bit     $C019
        bmi     :-

        ; Wait for bit 7 to be on (start of VBL)
:       bit     $C019
        bpl     :-

        ldx     #0
        ldy     #0
wait_vbl1:  ; now wait for bit 7 to be off again
        inx                       ; 2
        bne :+                    ; 5
        iny
:       bit     $C019             ; 9
        bmi     wait_vbl1         ; 12 (12 cycles per loop, 3000 cycles per Y incr)
wait_vbl2:  ; now wait for bit 7 to be on again (start of next VBL)
        inx                       ; 2
        bne :+                    ; 5
        iny
:       bit     $C019             ; 9
        bpl     wait_vbl2         ; 12 (12 cycles per loop, 3000 cycles per Y incr)

calibrate_done:
        lda     #60               ; Consider we're at 60Hz
        sta     hz
        cpy     #$06              ; But if Y = $06, we spent about 7*3000 cycles
        bne     :+                ; waiting for the interrupt: ~21000 cycles, so
        lda     #50               ; we're at 50Hz
        sta     hz
:       rts

        .bss

keyboard_level_change: .res 1
kbd_should_fire:       .res 1
