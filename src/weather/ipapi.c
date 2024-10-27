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

	surl_start_request(NULL, 0, ip_url, SURL_METHOD_GET);
	
	err = !surl_response_ok();
	handle_err("ip-api parse");

	surl_get_json(buf, ".status", "ISO646-FR1", 0, LINE_LEN);
	if (strcmp(buf, "success") != 0) {
		surl_get_json(buf, ".message", "ISO646-FR1", 0, LINE_LEN);
		sprintf(message, "ip-api(%s)", buf);
		err = 0xff;					// set unknown error 
		handle_err(message);
	}

	surl_get_json(loc->city, ".city", "ISO646-FR1", 0, HALF_LEN);
	surl_get_json(loc->countryCode, ".countryCode", "ISO646-FR1", 0, QUARTER_LEN);
	surl_get_json(loc->lon, ".lon", "ISO646-FR1", 0, HALF_LEN);
	surl_get_json(loc->lat, ".lat", "ISO646-FR1", 0, HALF_LEN);
}
