; Sanitizer table. Matches valid values to themselves,
; and invalid values to mid-range, in order to avoid
; jumping wrong and crashing in case a wrong byte is
; received.

.assert >_AUDIO_CODE_START = $40, error

safe_jumps:
        .repeat $40
        .byte >duty_cycle16
        .endrepeat
        .byte >duty_cycle0
        .byte >duty_cycle1
        .byte >duty_cycle2
        .byte >duty_cycle3
        .byte >duty_cycle4
        .byte >duty_cycle5
        .byte >duty_cycle6
        .byte >duty_cycle7
        .byte >duty_cycle8
        .byte >duty_cycle9
        .byte >duty_cycle10
        .byte >duty_cycle11
        .byte >duty_cycle12
        .byte >duty_cycle13
        .byte >duty_cycle14
        .byte >duty_cycle15
        .byte >duty_cycle16
        .byte >duty_cycle17
        .byte >duty_cycle18
        .byte >duty_cycle19
        .byte >duty_cycle20
        .byte >duty_cycle21
        .byte >duty_cycle22
        .byte >duty_cycle23
        .byte >duty_cycle24
        .byte >duty_cycle25
        .byte >duty_cycle26
        .byte >duty_cycle27
        .byte >duty_cycle28
        .byte >duty_cycle29
        .byte >duty_cycle30
        .byte >update_title
        .byte >break_out
        .repeat $9F
        .byte >duty_cycle16
        .endrepeat
