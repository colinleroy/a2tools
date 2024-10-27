#ifndef	WEATHERUI_H
#define WEATHERUI_H
#include "weatherdefs.h"
void handle_err();
void disp_menu(char *s);
void clear_display();
void change_location(LOCATION *loc);
void disp_message(char *msg);
void disp_nokey();
char get_keyin();
void progress_dots(char p);
#endif
