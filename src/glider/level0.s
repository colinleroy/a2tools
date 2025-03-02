        .export   level0_sprites, level0_blockers
        .export   level0_vents, level0_logic

        .import   _balloon, _balloon_mask
        .import   _clock, _clock_mask
        .import   _rubber_box, _rubber_box_mask

        .import   frame_counter
        .import   level_logic_done
        .import   _clock_inc_score

        .import   _grab_rubber_bands
        .import   _fire_balloon, _balloon_travel
        .import   plane_data, rubber_band_data

        .import   sprites_bgbackup

        .include  "balloon.gen.inc"
        .include  "clock.gen.inc"
        .include  "plane.gen.inc"
        .include  "rubber_box.gen.inc"
        .include  "sprite.inc"
        .include  "constants.inc"

.data

; Do not place anything after X= 224 to avoid overflow
; in the hitbox. For "unperfect" sprites, that can't be displayed
; at X%7 != 0, make sure x is a multiple of 7 so that the hitbox
; is aligned with the sprite.
level0_clock0_data:
                  .byte 1               ; active
                  .byte 0               ; deadly
                  .byte 0               ; destroyable
                  .byte 1               ; static
                  .byte 182             ; x
                  .byte clock_WIDTH
                  .byte 108             ; y
                  .byte clock_HEIGHT
                  .byte 182             ; prev_x
                  .byte 108             ; prev_y
                  .byte clock_BYTES-1   ; bytes of sprite - 1
                  .byte clock_BPLINE-1  ; width of sprite in bytes
                  .addr _clock          ; clock sprites
                  .addr _clock_mask     ; clock masks
                  .byte CLOCK_BONUS     ; deac cb data
                  .addr _clock_inc_score; deac cb
                  .word $0000           ; state backup
                  .addr sprites_bgbackup+0 ; bg backup
                  .byte 0               ; need clear

level0_balloon0_data:
                  .byte 0               ; active
                  .byte 1               ; deadly
                  .byte 1               ; destroyable
                  .byte 0               ; static
                  .byte 189             ; x
                  .byte balloon_WIDTH
                  .byte 191-balloon_HEIGHT
                  .byte balloon_HEIGHT
                  .byte 189             ; prev_x
                  .byte 191-balloon_HEIGHT
                  .byte balloon_BYTES-1 ; bytes of sprite - 1
                  .byte balloon_BPLINE-1; width of sprite in bytes
                  .addr _balloon        ; clock sprites
                  .addr _balloon_mask   ; clock masks
                  .byte 0
                  .addr $0000
                  .word $0000
                  .addr sprites_bgbackup+128
                  .byte 0               ; need clear

level0_rubber_box0_data:
                  .byte 1               ; active
                  .byte 0               ; deadly
                  .byte 0               ; destroyable
                  .byte 1               ; static
                  .byte 105             ; x
                  .byte rubber_box_WIDTH
                  .byte 91-rubber_box_HEIGHT
                  .byte rubber_box_HEIGHT
                  .byte 105             ; prev_x
                  .byte 91-rubber_box_HEIGHT
                  .byte rubber_box_BYTES-1 ; bytes of sprite - 1
                  .byte rubber_box_BPLINE-1; width of sprite in bytes
                  .addr _rubber_box        ; clock sprites
                  .addr _rubber_box_mask   ; clock masks
                  .byte 3
                  .addr _grab_rubber_bands ; deactivation callback
                  .word $0000              ; state backup
                  .addr sprites_bgbackup+256
                  .byte 0                  ; need clear

.rodata

; Sprite ordering counts.
; Plane must be last, rubber band must be first.
; In between, order for:
; - the biggest draw on EVEN frames as dashboard is updated on odd frames
; - the sprites higher in the screen (Y low) have less time to draw, racing
;   the beam: put them to be drawn first (at the end of the array, which is
;   walked backwards)
;
; Performance on 2025/02/26: even frames 10366 / odd frames 10760 cycles
level0_sprites:   .byte   5
level0_sprites_data:
                   ; Rubber band must be first for easy deactivation
                   ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small             x
                  .addr   level0_clock0_data        ; medium      x
                  .addr   level0_rubber_box0_data   ; medium            x
BALLOON_SPRITE_NUM = (*-level0_sprites_data)/2
                  .addr   level0_balloon0_data      ; big         x
                  .addr   plane_data                ; big         x     x

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
                  .byte   104, 24,  91,  31    ; Mac
                  .byte   103, 92,  121, 6     ; Table
                  .byte   148, 4,   126, 64    ; Table foot
                  .byte   0,   255, 191, 1     ; Floor

level0_logic:
        ; Move balloon if active
        lda     #BALLOON_SPRITE_NUM
        jsr     _balloon_travel

        ; Activate balloon if frame = $FF
        lda     #BALLOON_SPRITE_NUM
        ldx     #$FF
        jsr     _fire_balloon
        jmp     level_logic_done
