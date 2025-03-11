        .import   _chainsaw, _chainsaw_mask
        .import   _sheet, _sheet_mask
        .import   _switch, _switch_mask

        .import   level_logic_done

        .import   frame_counter
        .import   plane_data, rubber_band_data

        .import   _play_click, _play_chainsaw
        .import   _grab_sheet, _unfire_sprite

        .import   sprites_bgbackup

        .include  "plane.gen.inc"
        .include  "chainsaw.gen.inc"
        .include  "sheet.gen.inc"
        .include  "switch.gen.inc"
        .include  "level_data_struct.inc"
        .include  "code/sprite.inc"
        .include  "code/constants.inc"

.segment "level_f"

level_data:
                  .addr sprites
                  .addr vents
                  .addr blockers
                  .addr exits

.assert * = LEVEL_DATA_START+LEVEL_DATA::LOGIC_CB, error ; Make sure the callback is where we think
.proc logic
        ; Check if switch is active
        lda     switch0_data+SPRITE_DATA::ACTIVE
        bne     :+

        ; If not, activate the chainsaw every three frames
        lda     frame_counter
        and     #$03
        beq     :+
        sta     $C030
        sta     chainsaw0_data+SPRITE_DATA::ACTIVE
        rts

:       lda     chainsaw0_data+SPRITE_DATA::ACTIVE
        beq     :+
        lda     #CHAINSAW_SPRITE_NUM
        jsr     _unfire_sprite
        jsr     _play_chainsaw
:       rts
.endproc

; Do not place anything after X= 224 to avoid overflow
; in the hitbox
sheet0_data:
                  .byte 1              ; active
                  .byte 0              ; deadly
                  .byte 0              ; destroyable
                  .byte 1              ; static
                  .byte 175            ; x
                  .byte sheet_WIDTH
                  .byte 51-sheet_HEIGHT; y
                  .byte sheet_HEIGHT
                  .byte 175            ; prev_x
                  .byte 51-sheet_HEIGHT; prev_y
                  .byte sheet_BYTES-1  ; bytes of sprite - 1
                  .byte sheet_BPLINE-1 ; width of sprite in bytes
                  .addr _sheet         ; sprites
                  .addr _sheet_mask    ; masks
                  .byte 1
                  .addr _grab_sheet
                  .word $0000
                  .addr sprites_bgbackup+0
                  .byte 0               ; need clear

switch0_data:
                  .byte 1               ; active
                  .byte 0               ; deadly
                  .byte 0               ; destroyable
                  .byte 1               ; static
                  .byte 112             ; x
                  .byte switch_WIDTH
                  .byte 130             ; y
                  .byte switch_HEIGHT
                  .byte 112             ; prev_x
                  .byte 130             ; prev_y
                  .byte switch_BYTES-1  ; bytes of sprite - 1
                  .byte switch_BPLINE-1 ; width of sprite in bytes
                  .addr _switch         ; sprites
                  .addr _switch_mask    ; masks
                  .byte 0
                  .addr _play_click
                  .word $0000
                  .addr sprites_bgbackup+128
                  .byte 0               ; need clear

chainsaw0_data:
                  .byte 0               ; active
                  .byte 1               ; deadly
                  .byte 0               ; destroyable
                  .byte 0               ; static
                  .byte 140             ; x
                  .byte chainsaw_WIDTH
                  .byte 74              ; y
                  .byte chainsaw_HEIGHT
                  .byte 140             ; prev_x
                  .byte 74              ; prev_y
                  .byte chainsaw_BYTES-1  ; bytes of sprite - 1
                  .byte chainsaw_BPLINE-1 ; width of sprite in bytes
                  .addr _chainsaw         ; sprites
                  .addr _chainsaw_mask    ; masks
                  .byte 0
                  .addr $0000
                  .word $0000
                  .addr sprites_bgbackup+256
                  .byte 0               ; need clear

sprites:   .byte  5
sprites_data:
                   ; Rubber band must be first for easy deactivation
                   ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small            x
                  .addr   switch0_data              ; small       x
                  .addr   sheet0_data               ; medium           x
CHAINSAW_SPRITE_NUM = (*-sprites_data)/2
                  .addr   chainsaw0_data            ; big         x
                  .addr   plane_data                ; big         x    x

vents:     .byte  2
vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   9,   71,  plane_HEIGHT+1,    191-plane_HEIGHT, $02 ; Down all the way
                  .byte   148, 20,  plane_HEIGHT+1,    191-plane_HEIGHT, $FF ; Up all the way

blockers:  .byte  4
blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   204, 36,  14,  37    ; Box on top bookshelf
                  .byte   175, 78,  51,  4     ; Top bookshelf
                  .byte   175, 78,  86,  4     ; Lower bookshelf
                  .byte   0,   255, 191, 1     ; Floor

exits:     .byte  1
exits_data:
                  ; Seven bytes per exit (start X, width, start Y, height,
                  ; destination X, destination Y, destination level)
                  ; destination X or Y = $FF for no change
                  .byte   280-plane_WIDTH, 3,  0,  191, PLANE_ORIG_X, $FF, 'g'
