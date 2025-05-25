        .export         _oa_cgetc

        .import         _cgetc, machinetype

.proc _oa_cgetc
        jsr     _cgetc

        ; FIXME this should be in cc65
        bit     machinetype
        bmi     :+
        bit     $C061             ; Open-Apple?
        bpl     :+
        ora     #$80
:       rts

.endproc
