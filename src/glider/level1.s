        .export   level1_sprites, level1_blockers
        .export   level1_vents, level1_logic

        .import   _battery, _battery_mask
        .import   _clock, _clock_mask
        .import   _knife, _knife_mask

        .import   level_logic_done
        .import   _clock_inc_score
        .import   _grab_battery

        .import   _fire_sprite, _knife_travel

        .import   plane_data, rubber_band_data

        .import   sprites_bgbackup

        .include  "battery.gen.inc"
        .include  "clock.gen.inc"
        .include  "knife.gen.inc"
        .include  "plane.gen.inc"
        .include  "sprite.inc"
        .include  "constants.inc"

.data

; Do not place anything after X= 224 to avoid overflow
; in the hitbox
level1_battery0_data:
                  .byte 1               ; active
                  .byte 0               ; deadly
                  .byte 0               ; destroyable
                  .byte 1               ; static
                  .byte 105             ; x
                  .byte battery_WIDTH
                  .byte 51              ; y
                  .byte battery_HEIGHT
                  .byte 105             ; prev_x
                  .byte 51              ; y
                  .byte battery_BYTES-1   ; bytes of sprite - 1
                  .byte battery_BPLINE-1  ; width of sprite in bytes
                  .addr _battery          ; battery sprites
                  .addr _battery_mask     ; battery masks
                  .byte BATTERY_BONUS
                  .addr _grab_battery
                  .word $0000           ; state backup
                  .addr sprites_bgbackup+0
                  .byte 0               ; need clear

level1_clock0_data:
                  .byte 1              ; active
                  .byte 0              ; deadly
                  .byte 0              ; destroyable
                  .byte 1               ; static
                  .byte 189            ; x
                  .byte clock_WIDTH
                  .byte 50             ; y
                  .byte clock_HEIGHT
                  .byte 189            ; prev_x
                  .byte 50             ; prev_y
                  .byte clock_BYTES-1  ; bytes of sprite - 1
                  .byte clock_BPLINE-1 ; width of sprite in bytes
                  .addr _clock         ; clock sprites
                  .addr _clock_mask    ; clock masks
                  .byte CLOCK_BONUS+1
                  .addr _clock_inc_score
                  .word $0000
                  .addr sprites_bgbackup+128
                  .byte 0               ; need clear

level1_knife0_data:
                  .byte 0               ; active
                  .byte 1               ; deadly
                  .byte 1               ; destroyable
                  .byte 0               ; static
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
                  .addr sprites_bgbackup+256
                  .byte 0               ; need clear

level1_knife1_data:
                  .byte 0               ; active
                  .byte 1               ; deadly
                  .byte 1               ; destroyable
                  .byte 0               ; static
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
                  .addr sprites_bgbackup+384
                  .byte 0               ; need clear

.rodata

level1_sprites:   .byte   6
level1_sprites_data:
                   ; Rubber band must be first for easy deactivation
                   ;                                ; drawn on    EVEN ODD
                  .addr   rubber_band_data          ; small       x
                  .addr   level1_battery0_data      ; small            x
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
                  .byte   113, 67,  37,  29    ; Books
                  .byte   105, 99,  64,  4     ; Bookshelf
                  .byte   0,   255, 191, 1     ; Floor

.code

.proc level1_logic
        ; Move knives if active
        lda     #KNIFE0_SPRITE_NUM
        jsr     _knife_travel
        lda     #KNIFE1_SPRITE_NUM
        jsr     _knife_travel

        ; Activate knives
        lda     #KNIFE0_SPRITE_NUM
        ldx     #$30
        jsr     _fire_sprite

        lda     #KNIFE1_SPRITE_NUM
        ldx     #$70
        jmp     _fire_sprite
.endproc
