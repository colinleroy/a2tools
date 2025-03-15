        .import   plane_data, rubber_band_data

        .import   _sheet

        .import   _grab_sheet

        .import   sprites_bgbackup

        .include  "plane.gen.inc"
        .include  "sheet.gen.inc"
        .include  "level_data_struct.inc"
        .include  "code/sprite.inc"
        .include  "code/constants.inc"

.segment "level_k"

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
sheet0_data:
                  .byte 1              ; active
                  .byte 0              ; deadly
                  .byte 0              ; destroyable
                  .byte 1              ; static
                  .byte 217            ; x
                  .byte sheet_WIDTH
                  .byte 171-sheet_HEIGHT
                  .byte sheet_HEIGHT
                  .byte 217            ; prev_x
                  .byte 171-sheet_HEIGHT
                  .byte sheet_BYTES-1  ; bytes of sprite - 1
                  .byte sheet_BPLINE-1 ; width of sprite in bytes
                  .addr _sheet         ; sprites
                  .byte 1
                  .addr _grab_sheet
                  .word $0000
                  .addr sprites_bgbackup+0
                  .byte 0               ; need clear

sprites:   .byte  3
sprites_data:
                   ; Rubber band must be first for easy deactivation
                   ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small            x
                  .addr   sheet0_data               ; medium      x
                  .addr   plane_data                ; big         x    x

vents:     .byte  0
vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  ; .byte   152,   10,  plane_HEIGHT+1,    191-plane_HEIGHT, $FF ; Up all the way

blockers:  .byte  2
blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   77,  130 , 117, 67   ; Bathtub
                  .byte   0,   255, 191, 1     ; Floor

exits:     .byte  2
exits_data:
                  ; Seven bytes per exit (start X, width, start Y, height,
                  ; destination X, destination Y, destination level)
                  ; destination X or Y = $FF for no change
                  ; Use a non-existent level to win the game
                  .byte   210, 42, 170, 1, 183, 10, 'e'
                  .byte   280-plane_WIDTH, 3,  0,  191, PLANE_ORIG_X, $FF, 'j'
