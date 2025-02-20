        .export   level0_sprites, level0_blockers, level0_vents

        .import   clock0_backup
        .import   clock1_backup
        .import   clock2_backup

        .import   _clock, _clock_mask

        .export  level0_clock1_data

        .import   plane_data
        .include  "clock.inc"
        .include  "plane.inc"

.data

level0_clock0_data:
                  .byte 110           ; x
                  .byte 110           ; prev_x
                  .byte 107           ; y
                  .byte 107           ; prev_y
                  .byte clock_BYTES-1 ; bytes of sprite - 1
                  .byte clock_WIDTH/7 ; width of sprite in bytes
                  .addr clock0_backup ; background buffer
                  .addr _clock        ; clock sprites
                  .addr _clock_mask   ; clock masks

level0_clock1_data:
                  .byte 140           ; x
                  .byte 140           ; prev_x
                  .byte 107           ; y
                  .byte 107           ; prev_y
                  .byte clock_BYTES-1 ; bytes of sprite - 1
                  .byte clock_WIDTH/7 ; width of sprite in bytes
                  .addr clock1_backup ; background buffer
                  .addr _clock        ; clock sprites
                  .addr _clock_mask   ; clock masks

level0_clock2_data:
                  .byte 240           ; x
                  .byte 240           ; prev_x
                  .byte 177           ; y
                  .byte 177           ; prev_y
                  .byte clock_BYTES-1 ; bytes of sprite - 1
                  .byte clock_WIDTH/7 ; width of sprite in bytes
                  .addr clock2_backup ; background buffer
                  .addr _clock        ; clock sprites
                  .addr _clock_mask   ; clock masks

.rodata

level0_sprites:   .byte   4
level0_sprites_data:
                  .addr   plane_data
                  .addr   level0_clock0_data
                  .addr   level0_clock1_data
                  .addr   level0_clock2_data

level0_vents:     .byte   2
level0_vents_data:
                  ; Five bytes per vent (start X, end X, start Y, end Y, direction)
                  ; Direction = What to add to mouse_y
                  .byte   37-plane_WIDTH,  57,  0,   192, $FF ; Up all the way
                  .byte   227-plane_WIDTH, 247, 0,   192, $FF ; Up all the way

                  ; Examples of vents
                  ;.byte   41-plane_WIDTH,  61,  0,   121-plane_HEIGHT, $FF ; Up from desk height to top
                  ;.byte   102-plane_WIDTH, 126, 0,   192             , $2  ; Fast down all the way
                  ;.byte   200-plane_WIDTH, 220, 128, 192             , $FF ; Up from bottom to 128

level0_blockers:  .byte   4
level0_blockers_data:
                  ; Four bytes per blocker (start X, end X, start Y, end Y)
                  .byte   102-plane_WIDTH, 126, 91-plane_HEIGHT,  127    ; Mac
                  .byte   102-plane_WIDTH, 194, 121-plane_HEIGHT, 127    ; Desk
                  .byte   147-plane_WIDTH, 150, 126-plane_HEIGHT, 192    ; Foot
                  .byte   0,               255, 191-plane_HEIGHT, 191    ; Floor
