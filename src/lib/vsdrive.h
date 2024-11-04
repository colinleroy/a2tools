#ifndef __vsdrive_h
#define __vsdrive_h

#ifdef __CC65__

void __fastcall__  vsdrive_install(void);
void __fastcall__  vsdrive_uninstall(void);

#else

#define vsdrive_install()
#define vsdrive_uninstall()

#endif

#endif
