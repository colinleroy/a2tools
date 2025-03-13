        .import   _clock
        .import   _switch
        .import   _socket

        .import   _clock_inc_score

        .import   plane_data, rubber_band_data

        .import   _socket_toggle

        .import   sprites_bgbackup

        .import   _play_click

        .include  "clock.gen.inc"
        .include  "plane.gen.inc"
        .include  "socket.gen.inc"
        .include  "switch.gen.inc"
        .include  "level_data_struct.inc"
        .include  "code/sprite.inc"
        .include  "code/constants.inc"

.segment "level_c"

level_data:
                  .addr sprites
                  .addr vents
                  .addr blockers
                  .addr exits

.assert * = LEVEL_DATA_START+LEVEL_DATA::LOGIC_CB, error ; Make sure the callback is where we think
.proc logic
        ; Check if switch is active and toggle socket accordingly
        ldx     switch0_data+SPRITE_DATA::ACTIVE
        lda     #SOCKET_SPRITE_NUM
        jmp     _socket_toggle
.endproc

; Do not place anything after X= 224 to avoid overflow
; in the hitbox
clock0_data:
                  .byte 1              ; active
                  .byte 0              ; deadly
                  .byte 0              ; destroyable
                  .byte 1               ; static
                  .byte 147            ; x
                  .byte clock_WIDTH
                  .byte 68             ; y
                  .byte clock_HEIGHT
                  .byte 147            ; prev_x
                  .byte 68             ; prev_y
                  .byte clock_BYTES-1  ; bytes of sprite - 1
                  .byte clock_BPLINE-1 ; width of sprite in bytes
                  .addr _clock         ; sprites
                  .byte CLOCK_BONUS+2
                  .addr _clock_inc_score
                  .word $0000
                  .addr sprites_bgbackup+0
                  .byte 0               ; need clear

socket0_data:
                  .byte 1              ; active
                  .byte 1              ; deadly
                  .byte 0              ; destroyable
                  .byte 0              ; static
                  .byte 147            ; x
                  .byte socket_WIDTH
                  .byte 110            ; y
                  .byte socket_HEIGHT
                  .byte 147            ; prev_x
                  .byte 110            ; prev_y
                  .byte socket_BYTES-1 ; bytes of sprite - 1
                  .byte socket_BPLINE-1; width of sprite in bytes
                  .addr _socket        ; sprites
                  .byte 0
                  .addr $0000
                  .word $0000
                  .addr sprites_bgbackup+128
                  .byte 0               ; need clear

switch0_data:
                  .byte 1              ; active
                  .byte 0              ; deadly
                  .byte 0              ; destroyable
                  .byte 1               ; static
                  .byte 84             ; x
                  .byte switch_WIDTH
                  .byte 111            ; y
                  .byte switch_HEIGHT
                  .byte 84             ; prev_x
                  .byte 111            ; prev_y
                  .byte switch_BYTES-1 ; bytes of sprite - 1
                  .byte switch_BPLINE-1; width of sprite in bytes
                  .addr _switch        ; sprites
                  .byte 0
                  .addr _play_click
                  .word $0000
                  .addr sprites_bgbackup+256
                  .byte 0               ; need clear

sprites:   .byte  5
sprites_data:
                  ; Rubber band must be first for easy deactivation
                  ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small            x
                  .addr   clock0_data               ; medium      x
                  .addr   switch0_data              ; medium           x
SOCKET_SPRITE_NUM = (*-sprites_data)/2
                  .addr   socket0_data              ; medium      x
                  .addr   plane_data                ; big         x    x

vents:     .byte  2
vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   52,  10,  plane_HEIGHT+1,   191-plane_HEIGHT, $FF ; Up all the way
                  .byte   226, 10,  plane_HEIGHT+87,  103-plane_HEIGHT, $FF ; Up all the way

blockers:  .byte  4
blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   190, 34,  54,  28    ; Books
                  .byte   148, 78,  82,  3     ; Bookshelf
                  .byte   84 , 72,  133, 6     ; Table
                  .byte   0,   255, 191, 1     ; Floor

exits:     .byte  2
exits_data:
                  ; Seven bytes per exit (start X, width, start Y, height,
                  ; destination X, destination Y, destination level)
                  ; destination X or Y = $FF for no change
                  .byte   0, 3,  0,  191, 270-plane_WIDTH, $FF, 'b'
                  .byte   280-plane_WIDTH, 3,  0,  191, PLANE_ORIG_X, $FF, 'd'
