        .import   plane_data, rubber_band_data

        .include  "clock.gen.inc"
        .include  "plane.gen.inc"
        .include  "level_data_struct.inc"
        .include  "code/sprite.inc"
        .include  "code/constants.inc"

.segment "level_n"

level_data:
                  .addr sprites
                  .addr vents
                  .addr blockers
                  .addr exits

.assert * = LEVEL_DATA_START+LEVEL_DATA::LOGIC_CB, error ; Make sure the callback is where we think
.proc logic
        rts
.endproc

sprites:   .byte  2
sprites_data:
                   ; Rubber band must be first for easy deactivation
                   ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small       x
                  .addr   plane_data                ; big         x    x

vents:     .byte  8
vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   19,   20,  plane_HEIGHT+30,   192-30-plane_HEIGHT, $FF ; Up all the way
                  .byte   49,   20,  plane_HEIGHT+70,   192-70-plane_HEIGHT, $FF ; Up all the way
                  .byte   79,   20,  plane_HEIGHT+40,   192-40-plane_HEIGHT, $FF ; Up all the way
                  .byte   109,  20,  plane_HEIGHT+90,   192-90-plane_HEIGHT, $FF ; Up all the way
                  .byte   139,  20,  plane_HEIGHT+100,  192-100-plane_HEIGHT, $FF ; Up all the way
                  .byte   169,  20,  plane_HEIGHT+50,   192-50-plane_HEIGHT, $FF ; Up all the way
                  .byte   199,  20,  plane_HEIGHT+20,   192-20-plane_HEIGHT, $FF ; Up all the way
                  .byte   222,  20,  plane_HEIGHT+60,   192-60-plane_HEIGHT, $FF ; Up all the way

blockers:  .byte  1
blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   0,   255, 191, 1    ; Floor

exits:     .byte  2
exits_data:
                  ; Seven bytes per exit (start X, width, start Y, height,
                  ; destination X, destination Y, destination level)
                  ; destination X or Y = $FF for no change
                  ; Use a non-existent level to win the game
                  .byte   0, 3,  0,  191, 270-plane_WIDTH, $FF, 'm'
                  .byte   280-plane_WIDTH, 3,  0,  191, PLANE_ORIG_X, $FF, 'o'
