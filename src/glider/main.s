        .export   _main
        .export   _hgr_low, _hgr_hi
        .export   frame_counter
        .export   level_logic_done

        .import   _exit
        .import   _init_hgr, _init_mouse, _load_bg
        .import   _init_hgr_base_addrs, _hgr_baseaddr
        .import   _bzero
        .import   pushax
        .import   _mod7_table
        .import   cur_level, num_levels
        .import   _draw_sprite, _clear_and_draw_sprite
        .import   _setup_sprite_pointer
        .import   _check_blockers, _check_vents
        .import   _check_mouse_bounds
        .import   _check_rubber_band_bounds
        .import   _load_bg, _restore_bg
        .import   _deactivate_current_sprite

        .import   level_backup
        .import   levels_logic, cur_level_logic

        .import   _fire_rubber_band
        .import   _rubber_band_travel

        .import   reset_mouse, mouse_b
        .import   sprite_data, plane_data, rubber_band_data
        .import   mouse_irq_ready

        .importzp _zp6, ptr2, ptr4

        .include  "apple2.inc"
        .include  "plane.inc"
        .include  "sprite.inc"
        .include  "level_data_ptr.inc"
        .include  "plane_coords.inc"

_main:

        lda     #<$2000
        ldx     #>$2000
        jsr     pushax

        lda     #<$4000
        ldx     #>$4000
        jsr     _bzero

        lda     #1
        jsr     _init_hgr

        jsr     _init_hgr_base_addrs

        jsr     _init_simple_hgr_addrs

        jsr     _init_mouse
        bcc     :+
        jmp     _exit

:       jsr     load_level

loop:
        ; the WAI of the poor
        ; because I don't understand how WAI works
        ; and I want to keep things 6502-ok
        lda     mouse_irq_ready
        beq     loop
        lda     #0
        sta     mouse_irq_ready
;
; Main game loop!
;
        inc     frame_counter
        ; Check coordinates and update them depending on vents
        jsr     _check_vents
        clc
        adc     plane_y
        cmp     #plane_MAX_Y
        bcc     :+

        ; We're on the floor
        lda     #plane_MAX_Y

:       sta     plane_y

        ; Check obstacles
        jsr     _check_blockers
        bcc     move_checks_done

        ; We got in an obstacle
die:
        lda     $C030
        jsr     reset_level

move_checks_done:
        lda     plane_x
        cmp     #(280-plane_WIDTH)
        bne     level_logic
        inc     cur_level
        lda     cur_level
        cmp     num_levels
        bne     :+
        jmp     _win

:       jsr     load_level

level_logic:
        jmp     (cur_level_logic)

; The jump target back from level logic handler
level_logic_done:
        ; Check if we should fire a rubber band
        lda     mouse_b
        beq     :+
        jsr     _fire_rubber_band

:       jsr     _rubber_band_travel
        ldx     num_sprites
        dex
        txa
        sta     cur_sprite
        ; Always draw the plane
        jsr     _setup_sprite_pointer
        jsr     _clear_and_draw_sprite

        lda     frame_counter
        and     #01
        beq     :+                ; Draw only half sprites
        dec     cur_sprite
:

draw_next_sprite:
        dec     cur_sprite
        lda     cur_sprite
        bmi     loop              ; All done!

        jsr     _setup_sprite_pointer

        ldy     #SPRITE_DATA::ACTIVE
        lda     (cur_sprite_ptr),y
        beq     dec_sprite

        ; Let's check whether a rubber band can destroy this sprite
        lda     rubber_band_data+SPRITE_DATA::ACTIVE
        beq     :+
        ldy     #SPRITE_DATA::DESTROYABLE
        lda     (cur_sprite_ptr),y
        beq     :+

        ; We have an in-flight rubber band and that sprite is destroyable
        ldy     #SPRITE_DATA::X_COORD
        jsr     _check_rubber_band_bounds
        bcs     destroy_sprite

:       ; Let's check the sprite's box
        .assert data_ptr = cur_sprite_ptr, error
        ldy     #SPRITE_DATA::X_COORD
        jsr     _check_mouse_bounds
        bcc     :+

        ; We're in the sprite box, is it active?
        ldy     #SPRITE_DATA::ACTIVE
        lda     (cur_sprite_ptr),y
        beq     :+                ; No, we're good

        ; Is it deadly?
        ldy     #SPRITE_DATA::DEADLY
        lda     (cur_sprite_ptr),y
        bne     die               ; Yes, die

        ; Deactivate it
destroy_sprite:
        lda     cur_sprite
        jsr     _deactivate_current_sprite
        jmp     dec_sprite

:       jsr     _clear_and_draw_sprite

dec_sprite:
        dec     cur_sprite        ; Skip a sprite
        bpl     draw_next_sprite

        jmp     loop

; Copy the hgr_baseaddr array of addresses
; to two arrays of low bytes/high bytes for simplicity
_init_simple_hgr_addrs:
        ldy     #0
        ldx     #0
:       lda     _hgr_baseaddr,x
        sta     _hgr_low,y
        inx
        lda     _hgr_baseaddr,x
        sta     _hgr_hi,y
        iny
        inx
        bne     :-

:       lda     _hgr_baseaddr+256,x
        sta     _hgr_low,y
        inx
        lda     _hgr_baseaddr+256,x
        sta     _hgr_hi,y
        inx
        iny
        cpy     #192
        bne     :-

        ; Rewrite the mod7_table to have actual modulo-7s
        ; instead of a bit set, because we want to use it
        ; to select a sprite from [0-6] instead of ORing
        ; a bit in an HGR byte (cf ../lib/hgr_addrs.c)
        ldx     #0
        lda     #0
next_mod:
        cmp     #7
        bne     :+
        lda     #0
:       sta     _mod7_table,x
        clc
        adc     #1
        inx
        bne     next_mod

        rts

backup_sprite:
        lda     cur_sprite
        asl
        tay
        lda     (level_data),y
        sta     ptr2
        iny
        lda     (level_data),y
        sta     ptr2+1

        ldy     #.sizeof(SPRITE_DATA)
        dey

:       lda     (ptr2),y
        sta     level_backup,x
        inx
        dey
        bpl     :-
        rts

backup_level_data:
        ldx     num_sprites
        dex
        stx     cur_sprite

        ldx     #0
:       jsr     backup_sprite
        dec     cur_sprite
        bpl     :-

        ldx     num_sprites
        stx     cur_sprite

        rts

restore_sprite:
        lda     cur_sprite
        asl
        tay
        lda     (level_data),y
        sta     ptr2
        iny
        lda     (level_data),y
        sta     ptr2+1

        ldy     #.sizeof(SPRITE_DATA)
        dey

:       lda     level_backup,x
        sta     (ptr2),y
        inx
        dey
        bpl     :-
        rts

restore_level_data:
        ldx     num_sprites
        dex
        stx     cur_sprite

        ldx     #0
:       jsr     restore_sprite
        dec     cur_sprite
        bpl     :-

        ldx     num_sprites
        stx     cur_sprite

        jmp     _restore_bg

setup_level_data:
        lda     cur_level
        asl
        tax

        ; Logic
        lda     levels_logic,x
        sta     cur_level_logic
        lda     levels_logic+1,x
        sta     cur_level_logic+1

        ; Sprites
        lda     sprite_data,x
        sta     level_data
        lda     sprite_data+1,x
        sta     level_data+1

        ldy     #0
        lda     (level_data),y
        sta     num_sprites
        sta     cur_sprite

        ; Move pointer to actual data now we have the number
        ; of sprites
        inc     level_data
        bne     :+
        inc     level_data+1

:
        jsr     backup_level_data
        ; Draw each sprite once
first_draw:
        dec     cur_sprite
        lda     cur_sprite
        bmi     :+
        jsr     _setup_sprite_pointer
        jsr     _draw_sprite
        lda     cur_sprite
        jsr     _setup_sprite_pointer
        jsr     _clear_and_draw_sprite
        jmp     first_draw
:
        rts

reset_level:
        jsr     restore_level_data
        jsr     setup_level_data
        jmp     reset_mouse

load_level:
        jsr     _load_bg
        ; Draw plane once to backup background
        jsr     setup_level_data
        jmp     reset_mouse

_win:
        jmp     _exit

        .bss

_hgr_low:        .res 192
_hgr_hi:         .res 192
frame_counter:   .res 1
num_sprites:     .res 1
cur_sprite:      .res 1
