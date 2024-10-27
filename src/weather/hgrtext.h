#ifndef HGRTEXT_H
#define HGRTEXT_H
void init_hgr();
void set_text();
void set_row(char row);
void SetCursorRow( int row );
void SetCursorCol( int col );
void set_colrow(char col, char row);
void set_colrowYX(char col, char row);
void draw_string( char *text );
void draw_tile(char id);
#endif
