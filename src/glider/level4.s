        .export   level4_sprites, level4_blockers
        .export   level4_vents, level4_logic

        .import   _toast, _toast_mask
        .import   _sheet, _sheet_mask

        .import   level_logic_done
        .import   _deactivate_sprite
        .import   _grab_sheet

        .import   frame_counter
        .import   plane_data, rubber_band_data

        .import   _fire_sprite, _toast_travel

        .import   sprites_bgbackup

        .include  "plane.gen.inc"
        .include  "sheet.gen.inc"
        .include  "toast.gen.inc"
        .include  "sprite.inc"
        .include  "constants.inc"

.data

; Do not place anything after X= 224 to avoid overflow
; in the hitbox
level4_sheet0_data:
                  .byte 1              ; active
                  .byte 0              ; deadly
                  .byte 0              ; destroyable
                  .byte 1              ; static
                  .byte 98             ; x
                  .byte sheet_WIDTH
                  .byte 71             ; y
                  .byte sheet_HEIGHT
                  .byte 98             ; prev_x
                  .byte 71             ; prev_y
                  .byte sheet_BYTES-1  ; bytes of sprite - 1
                  .byte sheet_BPLINE-1 ; width of sprite in bytes
                  .addr _sheet         ; sprites
                  .addr _sheet_mask    ; masks
                  .byte 1
                  .addr _grab_sheet
                  .word $0000
                  .addr sprites_bgbackup+0
                  .byte 0               ; need clear

level4_toast0_data:
                  .byte 0               ; active
                  .byte 1               ; deadly
                  .byte 1               ; destroyable
                  .byte 0               ; static
                  .byte 154             ; x
                  .byte toast_WIDTH
                  .byte 105             ; y
                  .byte toast_HEIGHT
                  .byte 154             ; prev_x
                  .byte 105             ; prev_y
                  .byte toast_BYTES-1   ; bytes of sprite - 1
                  .byte toast_BPLINE-1  ; width of sprite in bytes
                  .addr _toast          ; toast sprites
                  .addr _toast_mask     ; toast masks
                  .byte 0
                  .addr $0000
                  .word $0000           ; state backup
                  .addr sprites_bgbackup+128
                  .byte 0               ; need clear

.rodata

level4_sprites:   .byte   4
level4_sprites_data:
                   ; Rubber band must be first for easy deactivation
                   ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small            x
                  .addr   level4_sheet0_data        ; medium      x
TOAST0_SPRITE_NUM = (*-level4_sprites_data)/2
                  .addr   level4_toast0_data        ; medium      x
                  .addr   plane_data                ; big         x    x

level4_vents:     .byte   4
level4_vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   12,  20,  plane_HEIGHT+1,    191-plane_HEIGHT, $FF ; Up all the way
                  .byte   41,  20,  plane_HEIGHT+1,    191-plane_HEIGHT, $02 ; Down all the way
                  .byte   133, 20,  plane_HEIGHT+135,  57-plane_HEIGHT, $FF ; Up to the table
                  .byte   221, 20,  plane_HEIGHT+1,    191-plane_HEIGHT, $FF ; Up all the way

level4_blockers:  .byte   4
level4_blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   49,  78,  85,  4     ; Bookshelf left
                  .byte   177, 35,  63,  32    ; Bookshelf right with books
                  .byte   139, 72,  127, 6     ; Table
                  .byte   0,   255, 191, 1     ; Floor

.code

level4_logic:
        ; Move toast if active
        lda     #TOAST0_SPRITE_NUM
        jsr     _toast_travel

        ; Activate toast
        lda     #TOAST0_SPRITE_NUM
        ldx     #$30
        jsr     _fire_sprite

        jmp     level_logic_done
