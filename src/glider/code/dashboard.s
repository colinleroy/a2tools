        .export   _draw_dashboard
        .export   _draw_level_end
        .export   _clear_hgr_screen, _wait_for_input

        .import   num_lives, num_rubber_bands, num_battery, cur_score, cur_level
        .import   time_counter, frame_counter
        .import   _print_char, _print_string
        .import   _quick_draw_mini_plane
        .import   _quick_draw_mini_score
        .import   _quick_draw_battery_reverted
        .import   _quick_draw_rubber_band_reverted

        .import   do_bin2dec_16bit, bcd_input, bcd_result_thousand

        .import   _play_ding, _platform_msleep, _sleep

        .import   _time_bonus_str
        .import   _your_score_str
        .import   _press_key_str
        .import   _no_level_str
        .import   _game_won_str
        .import   _game_lost_str

        .import   _mouse_check_fire
        .import   pushax
        .import   _bzero
        .importzp tmp1, tmp2

        .include  "apple2.inc"
        .include  "font.gen.inc"
        .include  "mini_plane.gen.inc"
        .include  "rubber_band_reverted.gen.inc"
        .include  "battery_reverted.gen.inc"

.segment "LOWCODE"

SCORE_ICON_X       = 37
SCORE_ICON_Y       = 1

MINI_PLANE_ICON_X  = 37
MINI_PLANE_ICON_Y  = 21

RUBBER_BAND_ICON_X = 37
RUBBER_BAND_ICON_Y = 41

BATTERY_ICON_X     = 37
BATTERY_ICON_Y     = 61

.proc _print_number
        sta     bcd_input
        stx     dest_x+1
        sty     dest_y+1
        jsr     do_bin2dec_16bit

        ldx     #2
        stx     cur_char+1
char:
        lda     bcd_result_thousand,x

dest_y:
        ldy     #$FF
dest_x:
        ldx     #$FF
        clc
        adc     #'0'
        jsr     _print_char
        dec     dest_x+1
        dec     cur_char+1
cur_char:
        ldx     #$FF
        bpl     char
        rts
.endproc

.proc _draw_dashboard
        lda     frame_counter     ; Draw dashboard on odd frames
        and     #01
        bne     :+
        rts

:       ; Lives
        ldx     #MINI_PLANE_ICON_X+2
        ldy     #MINI_PLANE_ICON_Y+mini_plane_HEIGHT+font_HEIGHT+1
        lda     #0
        sta     bcd_input+1
        lda     num_lives
        jsr     _print_number

        ldx     #MINI_PLANE_ICON_X
        ldy     #MINI_PLANE_ICON_Y
        jsr     _quick_draw_mini_plane

        ; Score
        ldx     #SCORE_ICON_X+2
        ldy     #SCORE_ICON_Y+rubber_band_reverted_HEIGHT+font_HEIGHT+1
        lda     cur_score+1
        sta     bcd_input+1
        lda     cur_score
        jsr     _print_number

        ldx     #SCORE_ICON_X
        ldy     #SCORE_ICON_Y
        jsr     _quick_draw_mini_score

        ; Rubber bands
        ldx     #RUBBER_BAND_ICON_X+2
        ldy     #RUBBER_BAND_ICON_Y+rubber_band_reverted_HEIGHT+font_HEIGHT+1
        lda     #0
        sta     bcd_input+1
        lda     num_rubber_bands
        jsr     _print_number

        ldx     #RUBBER_BAND_ICON_X
        ldy     #RUBBER_BAND_ICON_Y
        jsr     _quick_draw_rubber_band_reverted

        ; Battery
        ldx     #BATTERY_ICON_X+2
        ldy     #BATTERY_ICON_Y+battery_reverted_HEIGHT+font_HEIGHT+1
        lda     #0
        sta     bcd_input+1
        lda     num_battery
        jsr     _print_number

        ldx     #BATTERY_ICON_X
        ldy     #BATTERY_ICON_Y
        jmp     _quick_draw_battery_reverted
.endproc

; Enter with carry set to display game won message
.proc _draw_level_end
        lda     #$00
        sta     game_won

        bcc     :+
        inc     game_won
        inc     time_counter      ; It will be $FF there.
:       sta     displayed

print_time_bonus:
        ; Time bonus
        lda     #<_time_bonus_str
        ldx     #>_time_bonus_str
        jsr     pushax

        ldx     #17
        ldy     #80
        jsr     _print_string

        lda     #0                ; X, Y updated to end of string
        sta     bcd_input+1
        lda     time_counter
        jsr     _print_number
        dec     time_counter      ; Decrement here so we can print it at 0...

print_score:
        ; Score
        lda     #<_your_score_str
        ldx     #>_your_score_str
        jsr     pushax

        ldx     #17
        ldy     #89
        jsr     _print_string

        lda     cur_score+1
        sta     bcd_input+1
        lda     cur_score
        jsr     _print_number

        lda     game_won
        bne     print_level     ; We don't want to recount bonus if we won

        lda     displayed
        bne     :+
        lda     #1              ; Initial delay before starting to count
        ldx     #0
        jsr     _sleep
        inc     displayed

:       lda     time_counter      ; ...And still cash time bonus
        bmi     print_level
        inc     cur_score
        bne     :+
        inc     cur_score+1
:       jsr     _play_ding
        jmp     print_time_bonus

print_level:
        ; Level
        lda     #<_press_key_str
        ldx     #>_press_key_str
        jsr     pushax

        ldx     #6
        ldy     #107
        jsr     _print_string

        lda     #0
        sta     bcd_input+1
        lda     cur_level
        clc
        adc     #1                ; Levels are counted from zero
        jsr     _print_number

        lda     game_won
        beq     :+

        ; Display game won message
        lda     #<_no_level_str
        ldx     #>_no_level_str
        jsr     pushax

        ldx     #6
        ldy     #143
        jsr     _print_string

        lda     #0
        sta     bcd_input+1
        lda     cur_level
        clc
        adc     #1                ; Levels are counted from zero
        jsr     _print_number


        lda     #<_game_won_str
        ldx     #>_game_won_str
        jsr     pushax

        ldx     #6
        ldy     #161
        jsr     _print_string

:
        ; Fallthrough through _wait_for_input
.endproc

.proc _wait_for_input
        bit     KBDSTRB
:       lda     KBD               ; Wait for key or click
        bmi     :+
        jsr     _mouse_check_fire
        bcc     :-
:       bit     KBDSTRB
        rts

.endproc

.proc _clear_hgr_screen
        lda     #<$2000
        ldx     #>$2000
        jsr     pushax

        lda     #<$2000
        ldx     #>$2000
        jmp     _bzero
.endproc

        .bss

displayed:        .res 1
game_won:         .res 1
