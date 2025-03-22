        .import   plane_data, rubber_band_data

        .import   _balloon, _clock

        .import   _clock_inc_score
        .import   _fire_sprite, _balloon_travel

        .import   sprites_bgbackup

        .include  "balloon.gen.inc"
        .include  "clock.gen.inc"
        .include  "plane.gen.inc"
        .include  "level_data_struct.inc"
        .include  "code/sprite.inc"
        .include  "code/constants.inc"

.segment "level_l"

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
                  .byte 154             ; x
                  .byte balloon_WIDTH
                  .byte 191-balloon_HEIGHT
                  .byte balloon_HEIGHT
                  .byte 154             ; prev_x
                  .byte 191-balloon_HEIGHT
                  .byte balloon_BYTES-1 ; bytes of sprite - 1
                  .byte balloon_BPLINE-1; width of sprite in bytes
                  .addr _balloon        ; sprites
                  .byte 0
                  .addr $0000
                  .word $0000
                  .addr sprites_bgbackup+0
                  .byte 0               ; need clear

clock0_data:
                  .byte 1               ; active
                  .byte 0               ; deadly
                  .byte 0               ; destroyable
                  .byte 1               ; static
                  .byte 105              ; x
                  .byte clock_WIDTH
                  .byte 96-clock_HEIGHT
                  .byte clock_HEIGHT
                  .byte 105              ; prev_x
                  .byte 96-clock_HEIGHT
                  .byte clock_BYTES-1   ; bytes of sprite - 1
                  .byte clock_BPLINE-1  ; width of sprite in bytes
                  .addr _clock          ; clock sprites
                  .byte CLOCK_AMOUNT    ; deac cb data
                  .addr _clock_inc_score; deac cb
                  .word $0000           ; state backup
                  .addr sprites_bgbackup+128 ; bg backup
                  .byte 0               ; need clear


sprites:   .byte  4
sprites_data:
                   ; Rubber band must be first for easy deactivation
                   ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small       x
                  .addr   clock0_data               ; medium           x
BALLOON_SPRITE_NUM = (*-sprites_data)/2
                  .addr   balloon0_data             ; big         x
                  .addr   plane_data                ; big         x    x

vents:     .byte  2
vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   15,   10,  plane_HEIGHT+1,    191-plane_HEIGHT, $FF ; Up all the way
                  .byte  159,   10,  plane_HEIGHT+1,    191-plane_HEIGHT, $FF ; Up all the way

blockers:  .byte  6
blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   40,  33 , 68, 106   ; High cupboard
                  .byte   86 , 36,  96, 74    ; boxes
                  .byte   115, 29,  22, 40    ; top cupboard 1
                  .byte   213, 29,  22, 40    ; top cupboard 2
                  .byte   184, 53, 123, 5     ; table
                  .byte   0,   255, 191, 1    ; Floor

exits:     .byte  2
exits_data:
                  ; Seven bytes per exit (start X, width, start Y, height,
                  ; destination X, destination Y, destination level)
                  ; destination X or Y = $FF for no change
                  ; Use a non-existent level to win the game
                  .byte   0, 3,  0,  191, 270-plane_WIDTH, $FF, 'h'
                  .byte   280-plane_WIDTH, 3,  0,  191, PLANE_ORIG_X, $FF, 'm'
