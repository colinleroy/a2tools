        .export _main
        .export   _hgr_low, _hgr_hi

        .import   _exit
        .import   _init_hgr, _init_mouse, _load_bg
        .import   _init_hgr_base_addrs, _hgr_baseaddr
        .import   _bzero
        .import   pushax
        .import   _mod7_table
        .import   cur_level
        .import   _draw_sprite, _clear_and_draw_sprite
        .import   _setup_sprite_pointer
        .import   _check_blockers, _check_vents
        .import   _check_mouse_bounds
        .import   _load_bg

        .import   level0_clock1_data

        .import   reset_mouse
        .import   sprite_data, plane_data
        .import   mouse_irq_ready

        .importzp _zp6, ptr4

        .include  "apple2.inc"
        .include  "plane.inc"
        .include  "sprite.inc"
        .include  "level_data_ptr.inc"
        .include  "plane_coords.inc"

_main:

        lda     #<$2000
        ldx     #>$2000
        jsr     pushax

        lda     #<$2000
        ldx     #>$2000
        jsr     _bzero

        lda     #1
        jsr     _init_hgr

        jsr     _init_hgr_base_addrs

        jsr     _init_simple_hgr_addrs

        jsr     _init_mouse
        bcs     err

        jsr     load_level

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
        jsr     reset_mouse

move_checks_done:
        inc     level0_clock1_data+SPRITE_DATA::X_COORD

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

:
        dec     cur_sprite
        lda     cur_sprite
        bmi     loop              ; All done!

        jsr     _setup_sprite_pointer
        ; Let's check the sprite's box
        .assert data_ptr = cur_sprite_ptr, error
        ldy     #SPRITE_DATA::X_COORD
        jsr     _check_mouse_bounds
        bcs     die

        jsr     _clear_and_draw_sprite

        dec     cur_sprite        ; Skip a sprite
        bpl     :-

        jmp     loop

err:
        jmp     _exit

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

setup_level_data:
        lda     cur_level
        asl
        tax
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

        ; Draw each sprite once
:
        dec     cur_sprite
        lda     cur_sprite
        bmi     :+
        jsr     _setup_sprite_pointer
        jsr     _draw_sprite
        jmp     :-
:
        rts

load_level:
        jsr     _load_bg
        ; Draw plane once to backup background
        jsr     setup_level_data
        jmp     reset_mouse

        .bss

_hgr_low:      .res 192
_hgr_hi:       .res 192
frame_counter: .res 1
num_sprites:   .res 1
cur_sprite:    .res 1
