        .export         _backup_screen_holes

        .importzp       ptr1

; Screen holes are at $[04,05,06,07][78-7F]
; and                 $[04,05,06,07][F8-FF]

_backup_screen_holes:
        sta     start_page+1
        sta     begin_page+1
        eor     #1
        sta     dest_page+1
        sta     end_page+1
        lda     #<$0478       ; Start at the first array
        ldx     #>$0478
        sta     ptr1
        stx     ptr1+1
begin_page:
        bit     $C054
copy_hole:
        ldy #7                ; do one array (7F=>78 or FF=>F8)
:       lda     (ptr1),y
dest_page:
        bit     $C055
        sta     (ptr1),y
start_page:
        bit     $C054
        dey
        bpl      :-
        lda     ptr1          ; Check if we just did $xxFx
        bmi     next_hole     ; Yes, so go do next page
        ora     #$80          ; No, so switch from $xx7x to $xxFx
        sta     ptr1
        jmp     copy_hole
next_hole:
        and     #$7F          ; Reset low byte to $78
        sta     ptr1
        inc     ptr1+1        ; Increment page
        lda     ptr1+1
        cmp     #$08
        bcc     copy_hole
end_page:
        bit     $C055
        rts
