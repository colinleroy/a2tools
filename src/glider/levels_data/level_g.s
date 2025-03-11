        .import   _socket, _socket_mask
        .import   _sheet, _sheet_mask

        .import   level_logic_done

        .import   frame_counter
        .import   plane_data, rubber_band_data

        .import   _socket_toggle

        .import   _play_click, _play_chainsaw
        .import   _grab_sheet, _unfire_sprite

        .import   sprites_bgbackup

        .include  "plane.gen.inc"
        .include  "socket.gen.inc"
        .include  "sheet.gen.inc"
        .include  "level_data_struct.inc"
        .include  "code/sprite.inc"
        .include  "code/constants.inc"

.segment "level_g"

level_data:
                  .addr sprites
                  .addr vents
                  .addr blockers
                  .addr exits

SOCKET0_FREQ = $40
SOCKET1_FREQ = $50
.assert * = LEVEL_DATA_START+LEVEL_DATA::LOGIC_CB, error ; Make sure the callback is where we think
.proc logic
        ; toggle socket0 every 30 frames
        dec     socket0_counter
        bne     :+
        lda     #SOCKET0_FREQ
        sta     socket0_counter
        lda     socket0_on
        and     #1
        eor     #1
        sta     socket0_on

:       ; and socket1 every 50
        dec     socket1_counter
        bne     :+
        lda     #SOCKET1_FREQ
        sta     socket1_counter
        lda     socket1_on
        and     #1
        eor     #1
        sta     socket1_on

:       ldx     socket0_on
        lda     #SOCKET0_SPRITE_NUM
        jsr     _socket_toggle

        ldx     socket1_on
        lda     #SOCKET1_SPRITE_NUM
        jmp     _socket_toggle
.endproc

socket0_counter: .byte SOCKET0_FREQ
socket1_counter: .byte SOCKET1_FREQ
socket0_on:      .byte 0
socket1_on:      .byte 0
noise:           .byte 0
; Do not place anything after X= 224 to avoid overflow
; in the hitbox
sheet0_data:
                  .byte 1              ; active
                  .byte 0              ; deadly
                  .byte 0              ; destroyable
                  .byte 1              ; static
                  .byte 133            ; x
                  .byte sheet_WIDTH
                  .byte 115-sheet_HEIGHT; y
                  .byte sheet_HEIGHT
                  .byte 133            ; prev_x
                  .byte 115-sheet_HEIGHT; prev_y
                  .byte sheet_BYTES-1  ; bytes of sprite - 1
                  .byte sheet_BPLINE-1 ; width of sprite in bytes
                  .addr _sheet         ; sprites
                  .addr _sheet_mask    ; masks
                  .byte 1
                  .addr _grab_sheet
                  .word $0000
                  .addr sprites_bgbackup+0
                  .byte 0               ; need clear

socket0_data:
                  .byte 0              ; active
                  .byte 1              ; deadly
                  .byte 0              ; destroyable
                  .byte 0              ; static
                  .byte 224            ; x
                  .byte socket_WIDTH
                  .byte 99            ; y
                  .byte socket_HEIGHT
                  .byte 224            ; prev_x
                  .byte 99            ; prev_y
                  .byte socket_BYTES-1 ; bytes of sprite - 1
                  .byte socket_BPLINE-1; width of sprite in bytes
                  .addr _socket        ; sprites
                  .addr _socket_mask   ; masks
                  .byte 0
                  .addr $0000
                  .word $0000
                  .addr sprites_bgbackup+128
                  .byte 0               ; need clear

socket1_data:
                  .byte 0              ; active
                  .byte 1              ; deadly
                  .byte 0              ; destroyable
                  .byte 0              ; static
                  .byte 224            ; x
                  .byte socket_WIDTH
                  .byte 140            ; y
                  .byte socket_HEIGHT
                  .byte 224            ; prev_x
                  .byte 140           ; prev_y
                  .byte socket_BYTES-1 ; bytes of sprite - 1
                  .byte socket_BPLINE-1; width of sprite in bytes
                  .addr _socket        ; sprites
                  .addr _socket_mask   ; masks
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
                  .addr   sheet0_data               ; medium      x
SOCKET0_SPRITE_NUM = (*-sprites_data)/2
                  .addr   socket0_data              ; medium           x
SOCKET1_SPRITE_NUM = (*-sprites_data)/2
                  .addr   socket1_data              ; medium      x
                  .addr   plane_data                ; big         x    x

vents:     .byte  1
vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   11,   20,  plane_HEIGHT+1,    191-plane_HEIGHT, $FF ; Up all the way

blockers:  .byte  3
blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   46,  112, 114, 60    ; Desk
                  .byte   68,  24,  84,  31    ; Macintosh
                  .byte   0,   255, 191, 1     ; Floor

exits:     .byte  2
exits_data:
                  ; Seven bytes per exit (start X, width, start Y, height,
                  ; destination X, destination Y, destination level)
                  ; destination X or Y = $FF for no change
                  ; Use a non-existent level to win the game
                  .byte   280-plane_WIDTH, 3,  0,  191, PLANE_ORIG_X, $FF, 'z'
                  .byte   0, 3,  0,  191, 270-plane_WIDTH, $FF, 'g'
