         .import _cputs, _cgetc, _clrscr
         .import _play_sample
         .import _countdown_8000_snd
         .import  _exit
         .export _main

.rodata

playing_str:   .byte "Playing...",$0D,$0A,$00
press_key_str: .byte "Press a key to play again",$0D,$0A,$00

.code

_main:
         jsr     _clrscr

:        lda     #<playing_str
         ldx     #>playing_str
         jsr     _cputs

         lda     #<_countdown_8000_snd
         ldx     #>_countdown_8000_snd
         ldy     #0
         jsr     _play_sample

         lda     #<press_key_str
         ldx     #>press_key_str
         jsr     _cputs
         jsr     _cgetc
         jmp     :-

         ; Unreachable
         jmp     _exit
