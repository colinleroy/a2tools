#ifndef	IPAPI_H
#define IPAPI_H
#include <stdbool.h>

// ip-api API call
void get_location(LOCATION *loc);
void handle_err(char *message);
#endif
