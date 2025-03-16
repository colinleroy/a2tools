        .import   plane_data, rubber_band_data

        .include  "clock.gen.inc"
        .include  "plane.gen.inc"
        .include  "level_data_struct.inc"
        .include  "code/sprite.inc"
        .include  "code/plane_coords.inc"
        .include  "code/constants.inc"

.segment "level_n"

level_data:
                  .addr sprites
                  .addr vents
                  .addr blockers
                  .addr exits

.assert * = LEVEL_DATA_START+LEVEL_DATA::LOGIC_CB, error ; Make sure the callback is where we think
.proc logic
        lda       plane_y         ; Update Y ourselves to make sure the plane
        clc                       ; floats free until player exits level
        adc       float_direction
        sta       plane_y

        cmp       #125
        bcs       go_up
        cmp       #50
        bcc       go_down
        rts
go_up:
        lda       #$FF
        sta       float_direction
        rts
go_down:
        lda       #$01
        sta       float_direction
        rts
.endproc

float_direction: .byte $01

sprites:   .byte  2
sprites_data:
                   ; Rubber band must be first for easy deactivation
                   ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small       x
                  .addr   plane_data                ; big         x    x

vents:     .byte  1
vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y

                  ; Set a level-wide vent that does not change Y so we can do
                  ; it in logic
                  .byte 0, 255, 0, 191, $00

blockers:  .byte  0
blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)

exits:     .byte  2
exits_data:
                  ; Seven bytes per exit (start X, width, start Y, height,
                  ; destination X, destination Y, destination level)
                  ; destination X or Y = $FF for no change
                  ; Use a non-existent level to win the game
                  .byte   0, 3,  0,  191, 270-plane_WIDTH, $FF, 'm'
                  .byte   280-plane_WIDTH, 3,  0,  191, PLANE_ORIG_X, $FF, 'o'
