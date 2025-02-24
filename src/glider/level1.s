        .export   level1_sprites, level1_blockers
        .export   level1_vents, level1_logic

        .import   _clock, _clock_mask

        .import   level_logic_done

        .import   plane_data, rubber_band_data
        .include  "clock.gen.inc"
        .include  "plane.gen.inc"
        .include  "sprite.inc"

.data

; Do not place anything after X= 224 to avoid overflow
; in the hitbox
level1_clock0_data:
                  .byte 1              ; active
                  .byte 0              ; deadly
                  .byte 0              ; destroyable
                  .byte 183            ; x
                  .byte clock_WIDTH
                  .byte 50             ; y
                  .byte clock_HEIGHT
                  .byte 183            ; prev_x
                  .byte 50             ; prev_y
                  .byte clock_BYTES-1  ; bytes of sprite - 1
                  .byte clock_WIDTH/7  ; width of sprite in bytes
                  .addr _clock         ; clock sprites
                  .addr _clock_mask    ; clock masks
                  .addr $0000

.rodata

level1_sprites:   .byte   3
level1_sprites_data:
                  .addr   rubber_band_data    ; Must be first for easy deactivation
                  .addr   level1_clock0_data
                  .addr   plane_data

level1_vents:     .byte   2
level1_vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   35,  20,  plane_HEIGHT+1,   191-plane_HEIGHT, $FF ; Up all the way
                  .byte   217, 20,  plane_HEIGHT+1,   191-plane_HEIGHT, $FF ; Up all the way

level1_blockers:  .byte   3
level1_blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   100, 67,  37,  29    ; Books
                  .byte   100, 99,  64,  3     ; Bookshelf
                  .byte   0,   255, 191, 1     ; Floor

level1_logic:
        jmp     level_logic_done
