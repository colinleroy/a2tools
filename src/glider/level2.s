        .export   level2_sprites, level2_blockers
        .export   level2_vents, level2_logic

        .import   _clock, _clock_mask
        .import   _switch, _switch_mask
        .import   _socket, _socket_mask

        .import   level_logic_done
        .import   _deactivate_sprite

        .import   frame_counter
        .import   plane_data
        .include  "clock.inc"
        .include  "plane.inc"
        .include  "socket.inc"
        .include  "switch.inc"
        .include  "sprite.inc"

.data

; Do not place anything after X= 224 to avoid overflow
; in the hitbox
level2_clock0_data:
                  .byte 1              ; active
                  .byte 0              ; deadly
                  .byte 149            ; x
                  .byte clock_WIDTH
                  .byte 68             ; y
                  .byte clock_HEIGHT
                  .byte 149            ; prev_x
                  .byte 68             ; prev_y
                  .byte clock_BYTES-1  ; bytes of sprite - 1
                  .byte clock_WIDTH/7  ; width of sprite in bytes
                  .addr _clock         ; sprites
                  .addr _clock_mask    ; masks

level2_socket0_data:
                  .byte 1              ; active
                  .byte 1              ; deadly
                  .byte 145            ; x
                  .byte socket_WIDTH
                  .byte 110            ; y
                  .byte socket_HEIGHT
                  .byte 145            ; prev_x
                  .byte 110            ; prev_y
                  .byte socket_BYTES-1 ; bytes of sprite - 1
                  .byte socket_WIDTH/7 ; width of sprite in bytes
                  .addr _socket        ; sprites
                  .addr _socket_mask   ; masks

level2_switch0_data:
                  .byte 1              ; active
                  .byte 0              ; deadly
                  .byte 84             ; x
                  .byte switch_WIDTH
                  .byte 111            ; y
                  .byte switch_HEIGHT
                  .byte 84             ; prev_x
                  .byte 111            ; prev_y
                  .byte switch_BYTES-1 ; bytes of sprite - 1
                  .byte switch_WIDTH/7 ; width of sprite in bytes
                  .addr _switch        ; sprites
                  .addr _switch_mask   ; masks

.rodata

level2_sprites:   .byte   4
level2_sprites_data:
                  .addr   level2_clock0_data
                  .addr   level2_switch0_data
SOCKET_SPRITE_NUM = (*-level2_sprites_data)/2
                  .addr   level2_socket0_data
                  .addr   plane_data

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
                  .byte   191, 34,  54,  28    ; Books
                  .byte   148, 78,  82,  3     ; Bookshelf
                  .byte   84 , 72,  133, 16    ; Table
                  .byte   119, 3,   139, 54    ; Table foot
                  .byte   0,   255, 191, 1     ; Floor

level2_logic:
        ; Check if switch is active
        lda     level2_switch0_data+SPRITE_DATA::ACTIVE
        beq     :+

        ; If so, activate the socket every two frames
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
