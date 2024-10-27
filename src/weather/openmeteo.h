#ifndef	OPENMETEO_H
#define OPENMETEO_H
#include <stdbool.h>

// function
void get_location(LOCATION *loc);

// OpenMeteo 
void setup_omurl();
char *time_str(char *buf);
bool om_geocoding(LOCATION *loc, char *city);
void get_om_info(LOCATION *loc, WEATHER *wi, FORECAST *fc);
void set_forecast1(FORECAST *fc);
void set_forecast2(FORECAST *fc);
#endif
