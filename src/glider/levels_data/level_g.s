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

.segment "level_g"

level_data:
                  .addr sprites
                  .addr vents
                  .addr blockers
                  .addr exits

.assert * = LEVEL_DATA_START+LEVEL_DATA::LOGIC_CB, error ; Make sure the callback is where we think
.proc logic
        rts
.endproc


battery0_data:
                  .byte 1               ; active
                  .byte 0               ; deadly
                  .byte 0               ; destroyable
                  .byte 1               ; static
                  .byte 217             ; x
                  .byte battery_WIDTH
                  .byte 99-battery_HEIGHT
                  .byte battery_HEIGHT
                  .byte 217             ; prev_x
                  .byte 99-battery_HEIGHT
                  .byte battery_BYTES-1   ; bytes of sprite - 1
                  .byte battery_BPLINE-1  ; width of sprite in bytes
                  .addr _battery          ; battery sprites
                  .byte BATTERY_BONUS
                  .addr _grab_battery
                  .word $0000           ; state backup
                  .addr sprites_bgbackup+0
                  .byte 0               ; need clear

sprites:   .byte  3
sprites_data:
                   ; Rubber band must be first for easy deactivation
                   ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small            x
                  .addr   battery0_data             ; medium      x
                  .addr   plane_data                ; big         x    x

vents:     .byte  2
vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   23,   10,  plane_HEIGHT+1,    191-plane_HEIGHT, $FF ; Up all the way
                  .byte   152,   10,  plane_HEIGHT+1,    191-plane_HEIGHT, $FF ; Up all the way

blockers:  .byte  2
blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   195, 30,  99,  76    ; Cupboard
                  .byte   0,   255, 191, 1     ; Floor

exits:     .byte  3
exits_data:
                  ; Seven bytes per exit (start X, width, start Y, height,
                  ; destination X, destination Y, destination level)
                  ; destination X or Y = $FF for no change
                  ; Use a non-existent level to win the game
                  .byte   0, 3,  0,  191, 270-plane_WIDTH, $FF, 'f'
                  .byte   280-plane_WIDTH, 3,  0,  191, PLANE_ORIG_X, $FF, 'h'
                  .byte   95, 92-plane_WIDTH, 25, 1, $FF, 163-plane_HEIGHT, 'i' ; stair exit
