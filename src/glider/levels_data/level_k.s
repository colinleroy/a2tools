        .import   plane_data, rubber_band_data

        .import   _balloon
        .import   _sheet

        .import   _grab_sheet
        .import   _fire_sprite, _balloon_travel

        .import   sprites_bgbackup

        .include  "balloon.gen.inc"
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
        ; Move balloon if active
        lda     #BALLOON_SPRITE_NUM
        jsr     _balloon_travel

        ; Activate balloon if frame = $FF
        lda     #BALLOON_SPRITE_NUM
        ldx     #$40
        jmp     _fire_sprite
.endproc

; Do not place anything after X= 224 to avoid overflow
; in the hitbox
balloon0_data:
                  .byte 0               ; active
                  .byte 1               ; deadly
                  .byte 1               ; destroyable
                  .byte 0               ; static
                  .byte 217             ; x
                  .byte balloon_WIDTH
                  .byte 180-balloon_HEIGHT
                  .byte balloon_HEIGHT
                  .byte 217             ; prev_x
                  .byte 180-balloon_HEIGHT
                  .byte balloon_BYTES-1 ; bytes of sprite - 1
                  .byte balloon_BPLINE-1; width of sprite in bytes
                  .addr _balloon        ; sprites
                  .byte 0
                  .addr $0000
                  .word $0000
                  .addr sprites_bgbackup+0
                  .byte 0               ; need clear

sheet0_data:
                  .byte 1              ; active
                  .byte 0              ; deadly
                  .byte 0              ; destroyable
                  .byte 1              ; static
                  .byte 49             ; x
                  .byte sheet_WIDTH
                  .byte 74-sheet_HEIGHT
                  .byte sheet_HEIGHT
                  .byte 49             ; prev_x
                  .byte 74-sheet_HEIGHT
                  .byte sheet_BYTES-1  ; bytes of sprite - 1
                  .byte sheet_BPLINE-1 ; width of sprite in bytes
                  .addr _sheet         ; sprites
                  .byte 1
                  .addr _grab_sheet
                  .word $0000
                  .addr sprites_bgbackup+128
                  .byte 0               ; need clear

sprites:   .byte  4
sprites_data:
                   ; Rubber band must be first for easy deactivation
                   ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small       x
                  .addr   sheet0_data               ; medium           x
BALLOON_SPRITE_NUM = (*-sprites_data)/2
                  .addr   balloon0_data             ; big         x
                  .addr   plane_data                ; big         x    x

vents:     .byte  1
vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   83,   10,  plane_HEIGHT+1,    191-plane_HEIGHT, $FF ; Up all the way

blockers:  .byte  3
blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   77,  126 , 117, 67   ; Bathtub
                  .byte   16,  57,   73, 4     ; Shelf
                  .byte   0,   255, 191, 1     ; Floor

exits:     .byte  2
exits_data:
                  ; Seven bytes per exit (start X, width, start Y, height,
                  ; destination X, destination Y, destination level)
                  ; destination X or Y = $FF for no change
                  ; Use a non-existent level to win the game
                  .byte   210, 42, 170, 1, 183, 10, 'e'
                  .byte   280-plane_WIDTH, 3,  0,  191, PLANE_ORIG_X, $FF, 'j'
