/*
 * # Weather application for FujiNet                     
 *
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <conio.h>

#include "weatherdefs.h"
#include "weatherdisp.h"
#include "ipapi.h"

#include "surl.h"
#include "strsplit.h"

extern int	err;

char ip_url[] = "http://ip-api.com/json/?fields=status,city,countryCode,lon,lat";


//
// get location info from ip 
// returns true: call is success
//         false:     is not success
//
void get_location(LOCATION *loc) {
	char	buf[LINE_LEN];
	char	message[LINE_LEN];
	char	n_lines;
	char	**lines;

	surl_start_request(NULL, 0, ip_url, SURL_METHOD_GET);
	
	err = !surl_response_ok();
	handle_err("ip-api parse");

	err = surl_get_json(large_buf, ".status,.city,.countryCode,.lon,.lat",
											"US-ASCII", 0, sizeof(large_buf));
	if (err > 0) {
		err = 0;
	}
	handle_err("Wrong JSON reply");

	n_lines = strsplit_in_place(large_buf, '\n', &lines);
	
	if (n_lines < 5 || strcmp(lines[0], "success") != 0) {
		surl_get_json(buf, ".message", "US-ASCII", 0, LINE_LEN);
		sprintf(message, "ip-api(%s)", buf);
		err = 0xff;					// set unknown error 
		handle_err(message);
	}

	strncpy(loc->city, lines[1], HALF_LEN);
	strncpy(loc->countryCode, lines[2], QUARTER_LEN);
	strncpy(loc->lon, lines[3], HALF_LEN);
	strncpy(loc->lat, lines[4], HALF_LEN);

	free(lines);
}
