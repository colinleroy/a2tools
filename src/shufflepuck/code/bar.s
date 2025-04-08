        .export     _choose_opponent

        .import     _mouse_wait_vbl, _mouse_check_button
        .import     _clear_sprite, _draw_sprite
        .import     _load_pointer_pointer
        .import     _setup_sprite_pointer_for_clear
        .import     _setup_sprite_pointer_for_draw
        
        .import     puck_x, mouse_x
        .import     puck_y, mouse_y
        .import     _puck_select
        .import     _check_keyboard

        .import     _exit

        .import     __OPPONENT_START__

        .include    "apple2.inc"
        .include    "sprite.inc"
        .include    "pointer.gen.inc"
        .include    "pointer_coords.inc"
        .include    "constants.inc"

.data
BOXES_START = *
opponents_boxes:                      ; X, W, Y, H, opponent number
        .byte 39,  17, 104, 26, 0     ; Skip
        .byte 196, 52, 172, 18, 1     ; Visine
        .byte 181, 54, 97,  48, 2     ; Vinnie
        .byte 15,  32, 68,  32, 3     ; Lexan
        .byte 73,  38, 94,  36, 4     ; Eneg
        .byte 231, 21, 13,  21, CH_ESC; Exit
NUM_OPPONENTS = (* - BOXES_START)/5

.segment "bar"

.proc _choose_opponent
wait_input:
        jsr     _mouse_wait_vbl

        lda     mouse_x
        sta     pointer_x
        lda     mouse_y
        sta     pointer_y
        jsr     clear_pointer
        jsr     draw_pointer
        jsr     _check_keyboard
        bcs     out
        jsr     _mouse_check_button
        bcc     wait_input

        jsr     find_opponent
        bcc     wait_input

out:
        cmp     #CH_ESC
        bne     :+
        jmp     _exit

:       pha
        jsr     clear_pointer
        pla
        rts
.endproc

.proc find_opponent
        ldy     #0
check_next_opponent:
        lda     opponents_boxes,y ; check left
        cmp     mouse_x
        iny
        bcs     skip_y
        adc     opponents_boxes,y ; check left+width
        cmp     mouse_x
        bcc     skip_y

        iny
        lda     opponents_boxes,y ; check top
        cmp     mouse_y
        iny
        bcs     skip_num_opponent
        adc     opponents_boxes,y ; check top+height
        cmp     mouse_y
        bcc     skip_num_opponent

        iny
        lda     opponents_boxes,y ; Opponent number
        sec
        rts

skip_y: iny
        iny
skip_num_opponent:
        iny
        iny
        cpy     #((NUM_OPPONENTS*5))
        bne     check_next_opponent
        clc
        rts
.endproc

.proc draw_pointer
        jsr     _load_pointer_pointer
        jsr     _setup_sprite_pointer_for_draw
        jmp     _draw_sprite
.endproc

.proc clear_pointer
        jsr     _load_pointer_pointer
        jsr     _setup_sprite_pointer_for_clear
        jmp     _clear_sprite
.endproc
