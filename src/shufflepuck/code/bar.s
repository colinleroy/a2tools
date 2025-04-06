        .export     _choose_opponent

        .import     _mouse_wait_vbl, _mouse_check_button
        .import     _clear_sprite, _draw_sprite
        .import     _load_puck_pointer
        .import     _setup_sprite_pointer_for_clear
        .import     _setup_sprite_pointer_for_draw
        
        .import     puck_x, mouse_x
        .import     puck_y, mouse_y
        .import     _puck_select
        .import     _check_keyboard
        .import     _play_bar

        .import     __OPPONENT_START__

        .include    "apple2.inc"
        .include    "sprite.inc"
        .include    "puck6.gen.inc"
        .include    "puck_coords.inc"

.data
BOXES_START = *
opponents_boxes:                      ; X, W, Y, H, opponent number
        .byte 39,  17, 104, 26, 0     ; Skip
        .byte 196, 52, 172, 18, 1     ; Visine
        .byte 181, 54, 97,  48, 2     ; Vinnie
        .byte 15,  32, 68,  32, 3     ; Lexan
NUM_OPPONENTS = (* - BOXES_START)/5

.segment "bar"

.proc _choose_opponent
        ldy     #0
        sty     puck_y
        jsr     _play_bar
        jsr     _puck_select

wait_input:
        jsr     _mouse_wait_vbl

        lda     mouse_x
        sta     puck_gx
        lda     mouse_y
        sta     puck_gy
        jsr     clear_pointer
        jsr     draw_pointer
        jsr     _check_keyboard
        bcs     out
        jsr     _mouse_check_button
        bcc     wait_input

        jsr     find_opponent
        bcc     wait_input

out:
        pha
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
        jsr     _load_puck_pointer
        jsr     _setup_sprite_pointer_for_draw
        jmp     _draw_sprite
.endproc

.proc clear_pointer
        jsr     _load_puck_pointer
        jsr     _setup_sprite_pointer_for_clear
        jmp     _clear_sprite
.endproc
