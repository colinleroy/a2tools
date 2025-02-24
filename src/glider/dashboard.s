        .export   _print_rubber_bands

        .import   rubber_band_x0, rubber_band_mask_x0
        .import   _print_char

        .import   _quick_draw_rubber_band_reverted

        .import   do_bin2dec_8bit, bcd_result_thousand

        .importzp tmp1, tmp2
        .include  "apple2.inc"
        .include  "font.gen.inc"
        .include  "rubber_band_reverted.gen.inc"

RUBBER_BAND_ICON_X = 37
RUBBER_BAND_ICON_Y = 1

_print_rubber_bands:
        jsr     do_bin2dec_8bit

        ldx     #2
        stx     cur_char+1
        lda     #RUBBER_BAND_ICON_X+2
        sta     dest_x+1

char:
        lda     bcd_result_thousand,x

        ldy     #RUBBER_BAND_ICON_Y+rubber_band_reverted_HEIGHT+font_HEIGHT+1
dest_x:
        ldx     #RUBBER_BAND_ICON_X
        jsr     _print_char
        dec     dest_x+1
        dec     cur_char+1
cur_char:
        ldx     #$FF
        bpl     char

print_done:
        ldx     #RUBBER_BAND_ICON_X
        ldy     #RUBBER_BAND_ICON_Y
        jmp     _quick_draw_rubber_band_reverted
