        .export   _print_lives, _print_rubber_bands

        .import   _print_char

        .import   _quick_draw_mini_plane
        .import   _quick_draw_rubber_band_reverted

        .import   do_bin2dec_8bit, bcd_result_thousand

        .importzp tmp1, tmp2
        .include  "apple2.inc"
        .include  "font.gen.inc"
        .include  "mini_plane.gen.inc"
        .include  "rubber_band_reverted.gen.inc"

MINI_PLANE_ICON_X  = 37
MINI_PLANE_ICON_Y  = 1

RUBBER_BAND_ICON_X = 37
RUBBER_BAND_ICON_Y = 20

_print_lives:
        jsr     do_bin2dec_8bit

        ldx     #2
        stx     lv_cur_char+1
        lda     #MINI_PLANE_ICON_X+2
        sta     lv_dest_x+1

lv_char:
        lda     bcd_result_thousand,x

        ldy     #MINI_PLANE_ICON_Y+mini_plane_HEIGHT+font_HEIGHT+1
lv_dest_x:
        ldx     #MINI_PLANE_ICON_X
        jsr     _print_char
        dec     lv_dest_x+1
        dec     lv_cur_char+1
lv_cur_char:
        ldx     #$FF
        bpl     lv_char

        ldx     #MINI_PLANE_ICON_X
        ldy     #MINI_PLANE_ICON_Y
        jmp     _quick_draw_mini_plane

_print_rubber_bands:
        jsr     do_bin2dec_8bit

        ldx     #2
        stx     rb_cur_char+1
        lda     #RUBBER_BAND_ICON_X+2
        sta     rb_dest_x+1

rb_char:
        lda     bcd_result_thousand,x

        ldy     #RUBBER_BAND_ICON_Y+rubber_band_reverted_HEIGHT+font_HEIGHT+1
rb_dest_x:
        ldx     #RUBBER_BAND_ICON_X
        jsr     _print_char
        dec     rb_dest_x+1
        dec     rb_cur_char+1
rb_cur_char:
        ldx     #$FF
        bpl     rb_char

        ldx     #RUBBER_BAND_ICON_X
        ldy     #RUBBER_BAND_ICON_Y
        jmp     _quick_draw_rubber_band_reverted
