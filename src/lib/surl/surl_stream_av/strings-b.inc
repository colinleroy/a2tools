.ifdef __APPLE2ENH__
controls_str:     .byte "Loading...",$0D,$0A
                  .byte "Controls: Space:      Play/Pause,             Esc:     Quit player,",$0D,$0A
                  .byte "          Left/Right: Rewind/Forward 10s,     Up/Down: Rewind/Forward 1m",$0D,$0A
                  .byte "          -/=/+:      Volume up/default/down  S:       Toggle speed/quality",$00
.else
controls_str:     .byte "Loading...",$0D,$0A
                  .byte "Space: play/pause, Esc: Quit player,",$0D,$0A
                  .byte "Left/right: Rew/Fwd 10s, U/J: 60s",$0D,$0A
                  .byte "-/=/+: Vol up/def/down, S: spd/quality",$00
.endif
