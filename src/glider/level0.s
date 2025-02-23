        .export   level0_sprites, level0_blockers
        .export   level0_vents, level0_logic

        .import   _balloon, _balloon_mask
        .import   _clock, _clock_mask

        .import   frame_counter
        .import   level_logic_done

        .import   _deactivate_sprite
        .import   plane_data, rubber_band_data
        .include  "balloon.inc"
        .include  "clock.inc"
        .include  "plane.inc"
        .include  "sprite.inc"

.data

; Do not place anything after X= 224 to avoid overflow
; in the hitbox
level0_clock0_data:
                  .byte 1               ; active
                  .byte 0               ; deadly
                  .byte 181             ; x
                  .byte clock_WIDTH
                  .byte 108             ; y
                  .byte clock_HEIGHT
                  .byte 181             ; prev_x
                  .byte 108             ; prev_y
                  .byte clock_BYTES-1   ; bytes of sprite - 1
                  .byte clock_WIDTH/7   ; width of sprite in bytes
                  .addr _clock          ; clock sprites
                  .addr _clock_mask     ; clock masks

level0_balloon0_data:
                  .byte 0               ; active
                  .byte 1               ; deadly
                  .byte 196             ; x
                  .byte balloon_WIDTH
                  .byte 170             ; y
                  .byte balloon_HEIGHT
                  .byte 196             ; prev_x
                  .byte 170             ; prev_y
                  .byte balloon_BYTES-1 ; bytes of sprite - 1
                  .byte balloon_WIDTH/7 ; width of sprite in bytes
                  .addr _balloon        ; clock sprites
                  .addr _balloon_mask   ; clock masks

.rodata

level0_sprites:   .byte   4
level0_sprites_data:
                  .addr   rubber_band_data    ; Must be first for easy deactivation
BALLOON_SPRITE_NUM = (*-level0_sprites_data)/2
                  .addr   level0_balloon0_data
                  .addr   level0_clock0_data
                  .addr   plane_data

level0_vents:     .byte   2
level0_vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   40,  20,  plane_HEIGHT+1,   191-plane_HEIGHT, $FF ; Up all the way
                  .byte   227, 20,  plane_HEIGHT+1,   191-plane_HEIGHT, $FF ; Up all the way

level0_blockers:  .byte   4
level0_blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   102, 23,  91,  31    ; Mac
                  .byte   102, 92,  121, 6     ; Desk
                  .byte   147, 3,   126, 64    ; Foot
                  .byte   0,   255, 191, 1     ; Floor

level0_logic:
        ; Check if balloon is active
        lda     level0_balloon0_data+SPRITE_DATA::ACTIVE
        beq     :+
        ; If so, up it
        dec     level0_balloon0_data+SPRITE_DATA::Y_COORD
        bne     :+
        ; If Y = 0, deactivate it
        lda     #0
        sta     level0_balloon0_data+SPRITE_DATA::ACTIVE
        lda     #BALLOON_SPRITE_NUM
        jsr     _deactivate_sprite
        jmp     level_logic_done

        ; Activate balloon at frame 0
:       lda     frame_counter
        cmp     #$FF
        bne     :+
        lda     #1
        sta     level0_balloon0_data+SPRITE_DATA::ACTIVE
        lda     #170
        sta     level0_balloon0_data+SPRITE_DATA::Y_COORD
:       jmp     level_logic_done
