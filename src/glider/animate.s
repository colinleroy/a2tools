                  .export  _animate_plane_crash

                  .import  _plane, _plane_mask, plane_data
                  .import  _plane_destroyed, _plane_destroyed_mask

                  .import  _clear_and_draw_sprite
                  .import  _setup_sprite_pointer, _load_sprite_pointer
                  .import  plane_sprite_num
                  .import  _div7_table, tosumula0, pusha0, pushax, popax
                  .import  _platform_msleep

                  .importzp tmp1, tmp2

                  .include "level_data_ptr.inc"
                  .include "sprite.inc"

; Sprite number in A, mask then sprite in TOS
_replace_sprite:
        jsr     _load_sprite_pointer

        jsr     popax
        ldy     #SPRITE_DATA::SPRITE_MASK
        sta     (cur_sprite_ptr),y
        iny
        txa
        sta     (cur_sprite_ptr),y

        jsr     popax
        ldy     #SPRITE_DATA::SPRITE
        sta     (cur_sprite_ptr),y
        iny
        txa
        sta     (cur_sprite_ptr),y

        rts

; Round a number to the closest lower multiple of seven
_round_to_seven:
        tax
        lda     _div7_table,x
        jsr     pusha0
        lda     #7
        jmp     tosumula0

_animate_plane_crash:
        ; Update X coord to make sure we can play the animation in order
        lda     plane_data+SPRITE_DATA::X_COORD
        jsr     _round_to_seven
        sta     plane_data+SPRITE_DATA::X_COORD

        ; Get the plane's sprite number (last in sprite table)
        ldx     plane_sprite_num
        stx     tmp2

        ; Change sprite data & mask
        lda     #<_plane_destroyed
        ldx     #>_plane_destroyed
        jsr     pushax
        lda     #<_plane_destroyed_mask
        ldx     #>_plane_destroyed_mask
        jsr     pushax
        lda     tmp2
        jsr     _replace_sprite

        ; Loop for seven images
        lda     #7
        sta     tmp1

        ; Disable interrupts
        php
        sei

:       lda     tmp2
        jsr     _load_sprite_pointer
        jsr     _setup_sprite_pointer
        jsr     _clear_and_draw_sprite
        inc     plane_data+SPRITE_DATA::X_COORD
        lda     #32
        jsr     _platform_msleep
        dec     tmp1
        bne     :-

        plp

        ; Change sprite back
        lda     #<_plane
        ldx     #>_plane
        jsr     pushax
        lda     #<_plane_mask
        ldx     #>_plane_mask
        jsr     pushax
        lda     tmp2
        jsr     _replace_sprite

        rts
