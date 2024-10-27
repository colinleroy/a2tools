#ifndef	WEATHERDISP_H
#define WEATHERDISP_H
void disp_weather(WEATHER  *wi);
void disp_forecast(FORECAST *fc, char p);
char icon_code(char code);
char padding_center(char *s);
#endif
