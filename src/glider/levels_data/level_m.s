        .import   plane_data, rubber_band_data

        .import   _clock

        .import   _clock_inc_score

        .import   sprites_bgbackup

        .include  "clock.gen.inc"
        .include  "plane.gen.inc"
        .include  "level_data_struct.inc"
        .include  "code/sprite.inc"
        .include  "code/constants.inc"

.segment "level_m"

level_data:
                  .addr sprites
                  .addr vents
                  .addr blockers
                  .addr exits

.assert * = LEVEL_DATA_START+LEVEL_DATA::LOGIC_CB, error ; Make sure the callback is where we think
.proc logic
        rts
.endproc

; Do not place anything after X= 224 to avoid overflow
; in the hitbox
clock0_data:
                  .byte 1               ; active
                  .byte 0               ; deadly
                  .byte 0               ; destroyable
                  .byte 1               ; static
                  .byte 161              ; x
                  .byte clock_WIDTH
                  .byte 110-clock_HEIGHT
                  .byte clock_HEIGHT
                  .byte 161              ; prev_x
                  .byte 110-clock_HEIGHT
                  .byte clock_BYTES-1   ; bytes of sprite - 1
                  .byte clock_BPLINE-1  ; width of sprite in bytes
                  .addr _clock          ; clock sprites
                  .byte CLOCK_AMOUNT    ; deac cb data
                  .addr _clock_inc_score; deac cb
                  .word $0000           ; state backup
                  .addr sprites_bgbackup+0 ; bg backup
                  .byte 0               ; need clear


sprites:   .byte  3
sprites_data:
                   ; Rubber band must be first for easy deactivation
                   ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small       x
                  .addr   clock0_data               ; medium           x
                  .addr   plane_data                ; big         x    x

vents:     .byte  1
vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   19,   10,  plane_HEIGHT+1,    191-plane_HEIGHT, $FF ; Up all the way

blockers:  .byte  3
blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   58,  144, 133, 44   ; Couch
                  .byte   79 , 100, 111, 25   ; Couch back
                  .byte   0,   255, 191, 1    ; Floor

exits:     .byte  2
exits_data:
                  ; Seven bytes per exit (start X, width, start Y, height,
                  ; destination X, destination Y, destination level)
                  ; destination X or Y = $FF for no change
                  ; Use a non-existent level to win the game
                  .byte   0, 3,  0,  191, 270-plane_WIDTH, $FF, 'l'
                  .byte   280-plane_WIDTH, 3,  0,  191, PLANE_ORIG_X, $FF, 'n'
