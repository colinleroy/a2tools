        .export     _read_key

        .include    "apple2.inc"

.proc _read_key
        bit     KBDSTRB
:       lda     KBD
        bpl     :-
        bit     KBDSTRB
        and     #$7F
        rts
.endproc
