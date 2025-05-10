
eta_open_str:     .asciiz "(Opening stream...)"
eta_long_str:     .asciiz "(More than 30m remaining)"
eta_secs_str:     .asciiz "(About %ds remaining)    "
playback_err_str: .asciiz "Playback error"
no_video_str:     .byte "Warning: proxy can't open video aux_tty",$0D,$0A
                  .byte "No video available.", $00
