        .export   _print_dashboard

        .import   num_lives, num_rubber_bands, num_battery, cur_score
        .import   _print_char
        .import   _quick_draw_mini_plane
        .import   _quick_draw_mini_score
        .import   _quick_draw_battery_reverted
        .import   _quick_draw_rubber_band_reverted

        .import   do_bin2dec_16bit, bcd_input, bcd_result_thousand

        .importzp tmp1, tmp2
        .include  "apple2.inc"
        .include  "font.gen.inc"
        .include  "mini_plane.gen.inc"
        .include  "rubber_band_reverted.gen.inc"
        .include  "battery_reverted.gen.inc"

SCORE_ICON_X       = 37
SCORE_ICON_Y       = 1

MINI_PLANE_ICON_X  = 37
MINI_PLANE_ICON_Y  = 21

RUBBER_BAND_ICON_X = 37
RUBBER_BAND_ICON_Y = 41

BATTERY_ICON_X     = 37
BATTERY_ICON_Y     = 61

_print_number:
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
        jsr     _print_char
        dec     dest_x+1
        dec     cur_char+1
cur_char:
        ldx     #$FF
        bpl     char
        rts

_print_dashboard:
        ; Lives
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
