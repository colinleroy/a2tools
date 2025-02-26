        .export   level1_sprites, level1_blockers
        .export   level1_vents, level1_logic

        .import   _clock, _clock_mask
        .import   _knife, _knife_mask

        .import   level_logic_done
        .import   _clock_inc_score

        .import   _fire_knife, _knife_travel

        .import   plane_data, rubber_band_data
        .include  "clock.gen.inc"
        .include  "knife.gen.inc"
        .include  "plane.gen.inc"
        .include  "sprite.inc"

.data

; Do not place anything after X= 224 to avoid overflow
; in the hitbox
level1_clock0_data:
                  .byte 1              ; active
                  .byte 0              ; deadly
                  .byte 0              ; destroyable
                  .byte 182            ; x
                  .byte clock_WIDTH
                  .byte 50             ; y
                  .byte clock_HEIGHT
                  .byte 182            ; prev_x
                  .byte 50             ; prev_y
                  .byte clock_BYTES-1  ; bytes of sprite - 1
                  .byte clock_BPLINE-1 ; width of sprite in bytes
                  .addr _clock         ; clock sprites
                  .addr _clock_mask    ; clock masks
                  .byte 5
                  .addr _clock_inc_score
                  .word $0000

level1_knife0_data:
                  .byte 0               ; active
                  .byte 1               ; deadly
                  .byte 1               ; destroyable
                  .byte 140             ; x
                  .byte knife_WIDTH
                  .byte 1               ; y
                  .byte knife_HEIGHT
                  .byte 140             ; prev_x
                  .byte 1               ; prev_y
                  .byte knife_BYTES-1   ; bytes of sprite - 1
                  .byte knife_BPLINE-1  ; width of sprite in bytes
                  .addr _knife          ; knife sprites
                  .addr _knife_mask     ; knife masks
                  .byte 0
                  .addr $0000
                  .word $0000           ; state backup

level1_knife1_data:
                  .byte 0               ; active
                  .byte 1               ; deadly
                  .byte 1               ; destroyable
                  .byte 175             ; x
                  .byte knife_WIDTH
                  .byte 1               ; y
                  .byte knife_HEIGHT
                  .byte 175             ; prev_x
                  .byte 1               ; prev_y
                  .byte knife_BYTES-1   ; bytes of sprite - 1
                  .byte knife_BPLINE-1  ; width of sprite in bytes
                  .addr _knife          ; knife sprites
                  .addr _knife_mask     ; knife masks
                  .byte 0
                  .addr $0000
                  .word $0000           ; state backup

.rodata

level1_sprites:   .byte   5
level1_sprites_data:
                   ; Rubber band must be first for easy deactivation
                   ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small            x
                  .addr   level1_clock0_data        ; medium      x
KNIFE1_SPRITE_NUM = (*-level1_sprites_data)/2
                  .addr   level1_knife1_data        ; medium           x
KNIFE0_SPRITE_NUM = (*-level1_sprites_data)/2
                  .addr   level1_knife0_data        ; medium      x
                  .addr   plane_data                ; big         x    x

level1_vents:     .byte   2
level1_vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   35,  20,  plane_HEIGHT+1,   191-plane_HEIGHT, $FF ; Up all the way
                  .byte   217, 20,  plane_HEIGHT+1,   191-plane_HEIGHT, $FF ; Up all the way

level1_blockers:  .byte   3
level1_blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   100, 67,  37,  29    ; Books
                  .byte   100, 99,  64,  3     ; Bookshelf
                  .byte   0,   255, 191, 1     ; Floor

level1_logic:
        ; Move knives if active
        lda     #KNIFE0_SPRITE_NUM
        jsr     _knife_travel
        lda     #KNIFE1_SPRITE_NUM
        jsr     _knife_travel

        ; Activate knives
        lda     #KNIFE0_SPRITE_NUM
        ldx     #$30
        jsr     _fire_knife

        lda     #KNIFE1_SPRITE_NUM
        ldx     #$70
        jsr     _fire_knife
        jmp     level_logic_done
