;
; Ullrich von Bassewitz, 06.08.1998
; Colin Leroy-Mira <colin@colino.net>, 2023
;
        .export         _zp1, _zp2, _zp3, _zp4, _zp5, _zp6, _zp7, _zp8, _zp9, _zp10, _zp11, _zp12, _zp13
        .export         _zp1s, _zp2s, _zp3s, _zp4s, _zp5s, _zp6s, _zp7s, _zp8s, _zp9s, _zp10s, _zp11s, _zp12s, _zp13s
        .export         _zp1p, _zp3p, _zp6p, _zp8p, _zp10p, _zp12p
        .export         _zp1ip, _zp3ip, _zp6ip, _zp8ip, _zp10ip, _zp12ip
        .export         _zp1i, _zp3i, _zp6i, _zp8i, _zp10i, _zp12i
        .export         _zp1si, _zp3si, _zp6si, _zp8si, _zp1si, _zp12si

; https://fadden.com/apple2/dl/zero-page.txt
; Of course don't use them badly like zp1i and zp2

_zp1  := $EB
_zp1i := $EB
_zp1s := $EB
_zp1si:= $EB
_zp1p := $EB
_zp1ip:= $EB
_zp2  := $EC
_zp2s := $EC

_zp3  := $ED
_zp3i := $ED
_zp3s := $ED
_zp3si:= $ED
_zp3p := $ED
_zp3ip:= $ED
_zp4  := $EE
_zp4s := $EE

_zp5  := $EF
_zp5s := $EF

_zp6  := $54
_zp6i := $54
_zp6s := $54
_zp6si:= $54
_zp6p := $54
_zp6ip:= $54
_zp7  := $55
_zp7s := $55

_zp8  := $56
_zp8i := $56
_zp8s := $56
_zp8si:= $56
_zp8p := $56
_zp8ip:= $56
_zp9  := $57
_zp9s := $57

_zp10 := $58
_zp10i:= $58
_zp10s := $58
_zp10si:= $58
_zp10p:= $58
_zp10ip:= $58
_zp11 := $59
_zp11s:= $59

_zp12 := $60
_zp12i:= $60
_zp12s:= $60
_zp12si:= $60
_zp12p:= $60
_zp12ip:= $60
_zp13 := $61
_zp13s:= $61
