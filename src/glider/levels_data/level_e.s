        .import   _toast, _toast_mask
        .import   _sheet, _sheet_mask

        .import   level_logic_done
        .import   _grab_sheet

        .import   frame_counter
        .import   plane_data, rubber_band_data

        .import   _fire_sprite, _toast_travel

        .import   sprites_bgbackup

        .include  "plane.gen.inc"
        .include  "sheet.gen.inc"
        .include  "toast.gen.inc"
        .include  "level_data_struct.inc"
        .include  "code/sprite.inc"
        .include  "code/constants.inc"

.segment "level_e"

level_data:
                  .addr sprites
                  .addr vents
                  .addr blockers
                  .addr exits

.assert * = LEVEL_DATA_START+LEVEL_DATA::LOGIC_CB, error ; Make sure the callback is where we think
.proc logic
        ; Move toast if active
        lda     #TOAST0_SPRITE_NUM
        jsr     _toast_travel

        ; Activate toast
        lda     #TOAST0_SPRITE_NUM
        ldx     #$30
        jmp     _fire_sprite
.endproc

; Do not place anything after X= 224 to avoid overflow
; in the hitbox
sheet0_data:
                  .byte 1              ; active
                  .byte 0              ; deadly
                  .byte 0              ; destroyable
                  .byte 1              ; static
                  .byte 98             ; x
                  .byte sheet_WIDTH
                  .byte 71             ; y
                  .byte sheet_HEIGHT
                  .byte 98             ; prev_x
                  .byte 71             ; prev_y
                  .byte sheet_BYTES-1  ; bytes of sprite - 1
                  .byte sheet_BPLINE-1 ; width of sprite in bytes
                  .addr _sheet         ; sprites
                  .addr _sheet_mask    ; masks
                  .byte 1
                  .addr _grab_sheet
                  .word $0000
                  .addr sprites_bgbackup+0
                  .byte 0               ; need clear

toast0_data:
                  .byte 0               ; active
                  .byte 1               ; deadly
                  .byte 1               ; destroyable
                  .byte 0               ; static
                  .byte 154             ; x
                  .byte toast_WIDTH
                  .byte 105             ; y
                  .byte toast_HEIGHT
                  .byte 154             ; prev_x
                  .byte 105             ; prev_y
                  .byte toast_BYTES-1   ; bytes of sprite - 1
                  .byte toast_BPLINE-1  ; width of sprite in bytes
                  .addr _toast          ; toast sprites
                  .addr _toast_mask     ; toast masks
                  .byte 0
                  .addr $0000
                  .word $0000           ; state backup
                  .addr sprites_bgbackup+128
                  .byte 0               ; need clear

sprites:   .byte  4
sprites_data:
                  ; Rubber band must be first for easy deactivation
                  ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small            x
                  .addr   sheet0_data        ; medium      x
TOAST0_SPRITE_NUM = (*-sprites_data)/2
                  .addr   toast0_data        ; medium      x
                  .addr   plane_data                ; big         x    x

vents:     .byte  4
vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   12,  20,  plane_HEIGHT+1,    191-plane_HEIGHT, $FF ; Up all the way
                  .byte   41,  20,  plane_HEIGHT+1,    191-plane_HEIGHT, $02 ; Down all the way
                  .byte   133, 20,  plane_HEIGHT+135,  57-plane_HEIGHT, $FF ; Up to the table
                  .byte   221, 20,  plane_HEIGHT+1,    191-plane_HEIGHT, $FF ; Up all the way

blockers:  .byte  4
blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   49,  78,  85,  4     ; Bookshelf left
                  .byte   177, 35,  63,  32    ; Bookshelf right with books
                  .byte   139, 72,  127, 6     ; Table
                  .byte   0,   255, 191, 1     ; Floor

exits:     .byte  2
exits_data:
                  ; Seven bytes per exit (start X, width, start Y, height,
                  ; destination X, destination Y, destination level)
                  ; destination X or Y = $FF for no change
                  .byte   280-plane_WIDTH, 3,  0,  191, PLANE_ORIG_X, $FF, 'f'
                  .byte   0, 3,  0,  191, 270-plane_WIDTH, $FF, 'd'
