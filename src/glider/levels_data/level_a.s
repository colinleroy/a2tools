        .import   _balloon
        .import   _clock
        .import   _rubber_box

        .import   _clock_inc_score

        .import   _grab_rubber_bands
        .import   _fire_sprite, _balloon_travel
        .import   plane_data, rubber_band_data

        .import   sprites_bgbackup

        .include  "balloon.gen.inc"
        .include  "clock.gen.inc"
        .include  "plane.gen.inc"
        .include  "rubber_box.gen.inc"
        .include  "level_data_struct.inc"
        .include  "code/sprite.inc"
        .include  "code/constants.inc"

.segment "level_a"

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
        ldx     #$FF
        jmp     _fire_sprite
.endproc

; Do not place anything after X= 224 to avoid overflow
; in the hitbox. For "unperfect" sprites, that can't be displayed
; at X%7 != 0, make sure x is a multiple of 7 so that the hitbox
; is aligned with the sprite.
clock0_data:
                  .byte 1               ; active
                  .byte 0               ; deadly
                  .byte 0               ; destroyable
                  .byte 1               ; static
                  .byte 182             ; x
                  .byte clock_WIDTH
                  .byte 108             ; y
                  .byte clock_HEIGHT
                  .byte 182             ; prev_x
                  .byte 108             ; prev_y
                  .byte clock_BYTES-1   ; bytes of sprite - 1
                  .byte clock_BPLINE-1  ; width of sprite in bytes
                  .addr _clock          ; clock sprites
                  .byte CLOCK_BONUS     ; deac cb data
                  .addr _clock_inc_score; deac cb
                  .word $0000           ; state backup
                  .addr sprites_bgbackup+0 ; bg backup
                  .byte 0               ; need clear

balloon0_data:
                  .byte 0               ; active
                  .byte 1               ; deadly
                  .byte 1               ; destroyable
                  .byte 0               ; static
                  .byte 189             ; x
                  .byte balloon_WIDTH
                  .byte 191-balloon_HEIGHT
                  .byte balloon_HEIGHT
                  .byte 189             ; prev_x
                  .byte 191-balloon_HEIGHT
                  .byte balloon_BYTES-1 ; bytes of sprite - 1
                  .byte balloon_BPLINE-1; width of sprite in bytes
                  .addr _balloon        ; sprites
                  .byte 0
                  .addr $0000
                  .word $0000
                  .addr sprites_bgbackup+128
                  .byte 0               ; need clear

rubber_box0_data:
                  .byte 1               ; active
                  .byte 0               ; deadly
                  .byte 0               ; destroyable
                  .byte 1               ; static
                  .byte 105             ; x
                  .byte rubber_box_WIDTH
                  .byte 88-rubber_box_HEIGHT
                  .byte rubber_box_HEIGHT
                  .byte 105             ; prev_x
                  .byte 88-rubber_box_HEIGHT
                  .byte rubber_box_BYTES-1 ; bytes of sprite - 1
                  .byte rubber_box_BPLINE-1; width of sprite in bytes
                  .addr _rubber_box        ; sprites
                  .byte 3
                  .addr _grab_rubber_bands ; deactivation callback
                  .word $0000              ; state backup
                  .addr sprites_bgbackup+256
                  .byte 0                  ; need clear

; Sprite ordering counts.
; Plane must be last, rubber band must be first.
; In between, order for:
; - the biggest draw on EVEN frames as dashboard is updated on odd frames
; - the sprites higher in the screen (Y low) have less time to draw, racing
;   the beam: put them to be drawn first (at the end of the array, which is
;   walked backwards)
;
; Performance on 2025/02/26: even frames 10366 / odd frames 10760 cycles
sprites:   .byte  5
sprites_data:
                  ; Rubber band must be first for easy deactivation
                  ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small             x
                  .addr   clock0_data        ; medium      x
                  .addr   rubber_box0_data   ; medium            x
BALLOON_SPRITE_NUM = (*-sprites_data)/2
                  .addr   balloon0_data      ; big         x
                  .addr   plane_data                ; big         x     x

vents:     .byte  2
vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   45,  10,  plane_HEIGHT+1,   191-plane_HEIGHT, $FF ; Up all the way
                  .byte   232, 10,  plane_HEIGHT+1,   191-plane_HEIGHT, $FF ; Up all the way

blockers:  .byte  4
blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   105, 33,  88,  24    ; Apple II screen
                  .byte   101, 57,  111, 11    ; Apple II bottom
                  .byte   103, 92,  121, 6     ; Table
                  .byte   0,   255, 191, 1     ; Floor

exits:     .byte  1
exits_data:
                  ; Seven bytes per exit (start X, width, start Y, height,
                  ; destination X, destination Y, destination level)
                  ; destination X or Y = $FF for no change
                  .byte   280-plane_WIDTH, 3,  0,  191, PLANE_ORIG_X, $FF, 'b'
