;
; Colin Leroy-Mira <colin@colino.net>, 2024
;
; unsigned int __fastcall__ prodos_dir_file_count(DIR *dir);
;

        .export   _prodos_dir_file_count

        .importzp ptr1

        .include  "apple2.inc"

.proc _prodos_dir_file_count
        sta       ptr1
        stx       ptr1+1

        ldy       #$26+$05
        lda       (ptr1),y
        tax
        dey
        lda       (ptr1),y
        rts
.endproc
