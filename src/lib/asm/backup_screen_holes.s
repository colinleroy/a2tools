        .export         _backup_screen_holes

        .importzp       ptr1

_backup_screen_holes:
        sta     start_page+1
        sta     begin_page+1
        eor     #1
        sta     dest_page+1
        sta     end_page+1
        lda     #<$0478
        ldx     #>$0478
        sta     ptr1
        stx     ptr1+1
begin_page:
        bit     $C054
copy_hole:
        ldy #7
:       lda     (ptr1),y
dest_page:
        bit     $C055
        sta     (ptr1),y
start_page:
        bit     $C054
        dey
        bpl      :-
        lda     ptr1
        bmi     next_hole
        ora     $80
        sta     ptr1
        jmp     copy_hole
next_hole:
        and     #$7F
        sta     ptr1
        inc     ptr1+1
        lda     ptr1+1
        cmp     #$08
        bcc     copy_hole
end_page:
        bit     $C055
        rts
