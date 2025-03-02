        .export   level3_sprites, level3_blockers
        .export   level3_vents, level3_logic

        .import   _clock, _clock_mask
        .import   _switch, _switch_mask
        .import   _socket, _socket_mask

        .import   level_logic_done
        .import   _deactivate_sprite
        .import   _clock_inc_score

        .import   frame_counter
        .import   plane_data, rubber_band_data

        .import   sprites_bgbackup

        .include  "clock.gen.inc"
        .include  "plane.gen.inc"
        .include  "sprite.inc"

.data

; Do not place anything after X= 224 to avoid overflow
; in the hitbox
level3_clock0_data:
                  .byte 1              ; active
                  .byte 0              ; deadly
                  .byte 0              ; destroyable
                  .byte 1              ; static
                  .byte 91             ; x
                  .byte clock_WIDTH
                  .byte 82             ; y
                  .byte clock_HEIGHT
                  .byte 91             ; prev_x
                  .byte 82             ; prev_y
                  .byte clock_BYTES-1  ; bytes of sprite - 1
                  .byte clock_BPLINE-1 ; width of sprite in bytes
                  .addr _clock         ; sprites
                  .addr _clock_mask    ; masks
                  .byte 5
                  .addr _clock_inc_score
                  .word $0000
                  .addr sprites_bgbackup+0
                  .byte 0               ; need clear

.rodata

level3_sprites:   .byte   3
level3_sprites_data:
                   ; Rubber band must be first for easy deactivation
                   ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small            x
                  .addr   level3_clock0_data        ; medium      x
                  .addr   plane_data                ; big         x    x

level3_vents:     .byte   3
level3_vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   12,  20,  plane_HEIGHT+1,  191-plane_HEIGHT, $FF ; Up all the way
                  .byte   90,  20,  plane_HEIGHT+1,  191-plane_HEIGHT, $02 ; Down all the way
                  .byte   189, 20,  plane_HEIGHT+1,  191-plane_HEIGHT, $FF ; Up all the way

level3_blockers:  .byte   4
level3_blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   52,  27,  69,  28    ; Books
                  .byte   51,  56,  97,  74    ; Cupboard 1
                  .byte   111, 57,  135, 36    ; Cupboard 2
                  .byte   0,   255, 191, 1     ; Floor

level3_logic:
        jmp     level_logic_done
