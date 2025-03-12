        .import   _battery, _battery_mask

        .import   _grab_battery

        .import   frame_counter
        .import   plane_data, rubber_band_data

        .import   _play_click, _play_chainsaw
        .import   _grab_sheet, _unfire_sprite

        .import   sprites_bgbackup

        .include  "plane.gen.inc"
        .include  "battery.gen.inc"
        .include  "level_data_struct.inc"
        .include  "code/sprite.inc"
        .include  "code/constants.inc"

.segment "level_i"

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
                  .addr   rubber_band_data          ; small            x
                  .addr   plane_data                ; big         x    x

vents:     .byte  1
vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   152,   10,  plane_HEIGHT+1,    191-plane_HEIGHT, $FF ; Up all the way

blockers:  .byte  2
blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   12,  72 , 128, 6     ; Table
                  .byte   0,   255, 191, 1     ; Floor

exits:     .byte  1
exits_data:
                  ; Seven bytes per exit (start X, width, start Y, height,
                  ; destination X, destination Y, destination level)
                  ; destination X or Y = $FF for no change
                  ; Use a non-existent level to win the game
                  .byte   101, 92-plane_WIDTH, 165-plane_HEIGHT, 1, $FF, 26, 'g' ; stair exit
