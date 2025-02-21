        .export _palette
        .export _palette_mask

        .include "palette.inc"

        .rodata

palette_x0:
            .byte 42	,	85	,	42	,	85	,	42	,	0
            .byte 85	,	42	,	85	,	42	,	85	,	0
            .byte 126	,	127	,	127	,	127	,	63	,	0
            .byte 125	,	127	,	127	,	127	,	95	,	0
            .byte 126	,	127	,	127	,	127	,	63	,	0
            .byte 125	,	127	,	127	,	127	,	95	,	0
            .byte 126	,	127	,	127	,	127	,	63	,	0
            .byte 125	,	127	,	127	,	127	,	95	,	0
            .byte 126	,	127	,	127	,	127	,	63	,	0
            .byte 125	,	127	,	127	,	127	,	95	,	0
            .byte 126	,	127	,	127	,	127	,	63	,	0
            .byte 125	,	127	,	127	,	127	,	95	,	0
            .byte 126	,	127	,	127	,	127	,	63	,	0
            .byte 85	,	42	,	85	,	42	,	85	,	0
            .byte 125	,	127	,	127	,	127	,	95	,	0
palette_mask_x0:
            .repeat 15
            .byte 0,	0,	0,	0,	0,	127
            .endrep

palette_x1:
            .byte 84	,	42	,	85	,	42	,	85	,	0
            .byte 42	,	85	,	42	,	85	,	42	,	1
            .byte 124	,	127	,	127	,	127	,	127	,	0
            .byte 122	,	127	,	127	,	127	,	63	,	1
            .byte 124	,	127	,	127	,	127	,	127	,	0
            .byte 122	,	127	,	127	,	127	,	63	,	1
            .byte 124	,	127	,	127	,	127	,	127	,	0
            .byte 122	,	127	,	127	,	127	,	63	,	1
            .byte 124	,	127	,	127	,	127	,	127	,	0
            .byte 122	,	127	,	127	,	127	,	63	,	1
            .byte 124	,	127	,	127	,	127	,	127	,	0
            .byte 122	,	127	,	127	,	127	,	63	,	1
            .byte 124	,	127	,	127	,	127	,	127	,	0
            .byte 42	,	85	,	42	,	85	,	42	,	1
            .byte 122	,	127	,	127	,	127	,	63	,	1
palette_mask_x1:
            .repeat 15
            .byte 1	,	0	,	0	,	0	,	0	,	126
            .endrep
palette_x2:
            .byte 40	,	85	,	42	,	85	,	42	,	1
            .byte 84	,	42	,	85	,	42	,	85	,	2
            .byte 120	,	127	,	127	,	127	,	127	,	1
            .byte 116	,	127	,	127	,	127	,	127	,	2
            .byte 120	,	127	,	127	,	127	,	127	,	1
            .byte 116	,	127	,	127	,	127	,	127	,	2
            .byte 120	,	127	,	127	,	127	,	127	,	1
            .byte 116	,	127	,	127	,	127	,	127	,	2
            .byte 120	,	127	,	127	,	127	,	127	,	1
            .byte 116	,	127	,	127	,	127	,	127	,	2
            .byte 120	,	127	,	127	,	127	,	127	,	1
            .byte 116	,	127	,	127	,	127	,	127	,	2
            .byte 120	,	127	,	127	,	127	,	127	,	1
            .byte 84	,	42	,	85	,	42	,	85	,	2
            .byte 116	,	127	,	127	,	127	,	127	,	2
palette_mask_x2:
            .repeat 15
            .byte 3	,	0	,	0	,	0	,	0	,	124
            .endrep

palette_x3:
            .byte 80	,	42	,	85	,	42	,	85	,	2
            .byte 40	,	85	,	42	,	85	,	42	,	5
            .byte 112	,	127	,	127	,	127	,	127	,	3
            .byte 104	,	127	,	127	,	127	,	127	,	5
            .byte 112	,	127	,	127	,	127	,	127	,	3
            .byte 104	,	127	,	127	,	127	,	127	,	5
            .byte 112	,	127	,	127	,	127	,	127	,	3
            .byte 104	,	127	,	127	,	127	,	127	,	5
            .byte 112	,	127	,	127	,	127	,	127	,	3
            .byte 104	,	127	,	127	,	127	,	127	,	5
            .byte 112	,	127	,	127	,	127	,	127	,	3
            .byte 104	,	127	,	127	,	127	,	127	,	5
            .byte 112	,	127	,	127	,	127	,	127	,	3
            .byte 40	,	85	,	42	,	85	,	42	,	5
            .byte 104	,	127	,	127	,	127	,	127	,	5
palette_mask_x3:
            .repeat 15
            .byte 7	,	0	,	0	,	0	,	0	,	120
            .endrep

palette_x4:
            .byte 32	,	85	,	42	,	85	,	42	,	5
            .byte 80	,	42	,	85	,	42	,	85	,	10
            .byte 96	,	127	,	127	,	127	,	127	,	7
            .byte 80	,	127	,	127	,	127	,	127	,	11
            .byte 96	,	127	,	127	,	127	,	127	,	7
            .byte 80	,	127	,	127	,	127	,	127	,	11
            .byte 96	,	127	,	127	,	127	,	127	,	7
            .byte 80	,	127	,	127	,	127	,	127	,	11
            .byte 96	,	127	,	127	,	127	,	127	,	7
            .byte 80	,	127	,	127	,	127	,	127	,	11
            .byte 96	,	127	,	127	,	127	,	127	,	7
            .byte 80	,	127	,	127	,	127	,	127	,	11
            .byte 96	,	127	,	127	,	127	,	127	,	7
            .byte 80	,	42	,	85	,	42	,	85	,	10
            .byte 80	,	127	,	127	,	127	,	127	,	11
palette_mask_x4:
            .repeat 15
            .byte 15	,	0	,	0	,	0	,	0	,	112
            .endrep

palette_x5:
            .byte 64	,	42	,	85	,	42	,	85	,	10
            .byte 32	,	85	,	42	,	85	,	42	,	21
            .byte 64	,	127	,	127	,	127	,	127	,	15
            .byte 32	,	127	,	127	,	127	,	127	,	23
            .byte 64	,	127	,	127	,	127	,	127	,	15
            .byte 32	,	127	,	127	,	127	,	127	,	23
            .byte 64	,	127	,	127	,	127	,	127	,	15
            .byte 32	,	127	,	127	,	127	,	127	,	23
            .byte 64	,	127	,	127	,	127	,	127	,	15
            .byte 32	,	127	,	127	,	127	,	127	,	23
            .byte 64	,	127	,	127	,	127	,	127	,	15
            .byte 32	,	127	,	127	,	127	,	127	,	23
            .byte 64	,	127	,	127	,	127	,	127	,	15
            .byte 32	,	85	,	42	,	85	,	42	,	21
            .byte 32	,	127	,	127	,	127	,	127	,	23
palette_mask_x5:
            .repeat 15
            .byte 31	,	0	,	0	,	0	,	0	,	96
            .endrep

palette_x6:
            .byte 0	,	85	,	42	,	85	,	42	,	21
            .byte 64	,	42	,	85	,	42	,	85	,	42
            .byte 0	,	127	,	127	,	127	,	127	,	31
            .byte 64	,	126	,	127	,	127	,	127	,	47
            .byte 0	,	127	,	127	,	127	,	127	,	31
            .byte 64	,	126	,	127	,	127	,	127	,	47
            .byte 0	,	127	,	127	,	127	,	127	,	31
            .byte 64	,	126	,	127	,	127	,	127	,	47
            .byte 0	,	127	,	127	,	127	,	127	,	31
            .byte 64	,	126	,	127	,	127	,	127	,	47
            .byte 0	,	127	,	127	,	127	,	127	,	31
            .byte 64	,	126	,	127	,	127	,	127	,	47
            .byte 0	,	127	,	127	,	127	,	127	,	31
            .byte 64	,	42	,	85	,	42	,	85	,	42
            .byte 64	,	126	,	127	,	127	,	127	,	47
palette_mask_x6:
            .repeat 15
            .byte 63	,	0	,	0	,	0	,	0	,	64
            .endrep

palette_x7:
            .byte 0	,	42	,	85	,	42	,	85	,	42
            .byte 0	,	85	,	42	,	85	,	42	,	85
            .byte 0	,	126	,	127	,	127	,	127	,	63
            .byte 0	,	125	,	127	,	127	,	127	,	95
            .byte 0	,	126	,	127	,	127	,	127	,	63
            .byte 0	,	125	,	127	,	127	,	127	,	95
            .byte 0	,	126	,	127	,	127	,	127	,	63
            .byte 0	,	125	,	127	,	127	,	127	,	95
            .byte 0	,	126	,	127	,	127	,	127	,	63
            .byte 0	,	125	,	127	,	127	,	127	,	95
            .byte 0	,	126	,	127	,	127	,	127	,	63
            .byte 0	,	125	,	127	,	127	,	127	,	95
            .byte 0	,	126	,	127	,	127	,	127	,	63
            .byte 0	,	85	,	42	,	85	,	42	,	85
            .byte 0	,	125	,	127	,	127	,	127	,	95
palette_mask_x7:
            .repeat 15
            .byte 127	,	0	,	0	,	0	,	0	,	0
            .endrep

_palette:
            .addr palette_x0
            .addr palette_x1
            .addr palette_x2
            .addr palette_x3
            .addr palette_x4
            .addr palette_x5
            .addr palette_x6
            .addr palette_x7

_palette_mask:
            .addr palette_mask_x0
            .addr palette_mask_x1
            .addr palette_mask_x2
            .addr palette_mask_x3
            .addr palette_mask_x4
            .addr palette_mask_x5
            .addr palette_mask_x6
            .addr palette_mask_x7
