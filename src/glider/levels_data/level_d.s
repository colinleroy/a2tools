        .import   _clock, _clock_mask

        .import   level_logic_done
        .import   _clock_inc_score

        .import   frame_counter
        .import   plane_data, rubber_band_data

        .import   sprites_bgbackup

        .include  "clock.gen.inc"
        .include  "plane.gen.inc"
        .include  "level_data_struct.inc"
        .include  "code/sprite.inc"
        .include  "code/constants.inc"

.segment "level_d"

level_data:
                  .addr sprites
                  .addr vents
                  .addr blockers

.assert * = LEVEL_DATA_START+LEVEL_DATA::LOGIC_CB, error ; Make sure the callback is where we think
.proc logic
        rts
.endproc

; Do not place anything after X= 224 to avoid overflow
; in the hitbox
clock0_data:
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
                  .byte CLOCK_BONUS+3
                  .addr _clock_inc_score
                  .word $0000
                  .addr sprites_bgbackup+0
                  .byte 0               ; need clear

sprites:   .byte   3
sprites_data:
                   ; Rubber band must be first for easy deactivation
                   ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small            x
                  .addr   clock0_data        ; medium      x
                  .addr   plane_data                ; big         x    x

vents:     .byte   3
vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   12,  20,  plane_HEIGHT+1,  191-plane_HEIGHT, $FF ; Up all the way
                  .byte   90,  20,  plane_HEIGHT+1,  191-plane_HEIGHT, $02 ; Down all the way
                  .byte   189, 20,  plane_HEIGHT+1,  191-plane_HEIGHT, $FF ; Up all the way

blockers:  .byte   4
blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   52,  27,  69,  28    ; Books
                  .byte   51,  56,  97,  74    ; Cupboard 1
                  .byte   111, 57,  135, 36    ; Cupboard 2
                  .byte   0,   255, 191, 1     ; Floor
