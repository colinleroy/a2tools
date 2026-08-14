Camera models supported:
 - Quicktake 100  (QT1X0.DRV)
 - Quicktake 150  (QT1X0.DRV)
 - Quicktake 200  (QT200.DRV)
 - Fujifilm  DS-7 (QT200.DRV)

Picture formats handled:
 - QKTK       (Quicktake 100)
 - Kodak RADC (Quicktake 150, Kodak DC50)
 - JPEG YH1V1 ()
 - JPEG YH2V1 (Quicktake 200, Epson PhotoPC, Sanyo VPC-G1)

Camera models to investigate:
 - Sanyo VPC-G200/210
   cable: http://www.alanwinstanley.com/alan-winstanleys-journal/2024/9/22/sanyo-vpc-g210-digicam-pc-connection.html, 
   proto: https://github.com/gphoto/libgphoto2/blob/master/camlibs/sierra/sierra.c
 - Kodak DC-50
   proto: https://github.com/colinleroy/kdcpi
 - Epson PhotoPC PCDC001 (aka Sanyo VPC-G1)
   maybe: https://photopc.sourceforge.net/cameras.html
 - Agfa ePhoto 307?
 - Casio QV-10/11/100/300? 
   proto: https://github.com/gphoto/libgphoto2/blob/master/camlibs/casio/casio-qv.c
   maybe: https://www.asahi-net.or.jp/~XG2K-HYS/index-e.html
 - Sony DSC-F1
 - Nikon Coolpix 300
