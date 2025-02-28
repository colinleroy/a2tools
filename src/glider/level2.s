        .export   level2_sprites, level2_blockers
        .export   level2_vents, level2_logic

        .import   _clock, _clock_mask
        .import   _switch, _switch_mask
        .import   _socket, _socket_mask

        .import   level_logic_done
        .import   _deactivate_sprite
        .import   _clock_inc_score

        .import   clock0_bgbackup
        .import   socket0_bgbackup
        .import   switch0_bgbackup

        .import   frame_counter
        .import   plane_data, rubber_band_data

        .import   _play_click

        .include  "clock.gen.inc"
        .include  "plane.gen.inc"
        .include  "socket.gen.inc"
        .include  "switch.gen.inc"
        .include  "sprite.inc"

.data

; Do not place anything after X= 224 to avoid overflow
; in the hitbox
level2_clock0_data:
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
                  .addr _clock_mask    ; masks
                  .byte 5
                  .addr _clock_inc_score
                  .word $0000
                  .addr clock0_bgbackup
                  .byte 0               ; need clear

level2_socket0_data:
                  .byte 1              ; active
                  .byte 1              ; deadly
                  .byte 0              ; destroyable
                  .byte 0               ; static
                  .byte 147            ; x
                  .byte socket_WIDTH
                  .byte 110            ; y
                  .byte socket_HEIGHT
                  .byte 147            ; prev_x
                  .byte 110            ; prev_y
                  .byte socket_BYTES-1 ; bytes of sprite - 1
                  .byte socket_BPLINE-1; width of sprite in bytes
                  .addr _socket        ; sprites
                  .addr _socket_mask   ; masks
                  .byte 0
                  .addr $0000
                  .word $0000
                  .addr socket0_bgbackup
                  .byte 0               ; need clear

level2_switch0_data:
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
                  .addr _switch_mask   ; masks
                  .byte 0
                  .addr _play_click
                  .word $0000
                  .addr switch0_bgbackup
                  .byte 0               ; need clear

.rodata

level2_sprites:   .byte   5
level2_sprites_data:
                   ; Rubber band must be first for easy deactivation
                   ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small            x
                  .addr   level2_clock0_data        ; medium      x
                  .addr   level2_switch0_data       ; medium           x
SOCKET_SPRITE_NUM = (*-level2_sprites_data)/2
                  .addr   level2_socket0_data       ; medium      x
                  .addr   plane_data                ; big         x    x

level2_vents:     .byte   2
level2_vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   47,  20,  plane_HEIGHT+1,   191-plane_HEIGHT, $FF ; Up all the way
                  .byte   221, 20,  plane_HEIGHT+87,  103-plane_HEIGHT, $FF ; Up all the way

level2_blockers:  .byte   5
level2_blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   190, 34,  54,  28    ; Books
                  .byte   148, 78,  82,  3     ; Bookshelf
                  .byte   84 , 72,  133, 16    ; Table
                  .byte   119, 4,   139, 53    ; Table foot
                  .byte   0,   255, 191, 1     ; Floor

level2_logic:
        ; Check if switch is active
        lda     level2_switch0_data+SPRITE_DATA::ACTIVE
        beq     :+

        ; If so, activate the socket every three frames
        lda     frame_counter
        and     #$03
        beq     :+
        sta     $C030
        sta     level2_socket0_data+SPRITE_DATA::ACTIVE
        jmp     level_logic_done

:       lda     level2_socket0_data+SPRITE_DATA::ACTIVE
        beq     :+
        lda     #SOCKET_SPRITE_NUM
        jsr     _deactivate_sprite
:       jmp     level_logic_done
