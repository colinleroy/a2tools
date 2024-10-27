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

#include "fujinet-network.h"
#include "weatherdefs.h"
#include "weatherdisp.h"
#include "ipapi.h"

extern int	err;

char ip_url[] = "N:http://ip-api.com/json/?fields=status,city,countryCode,lon,lat";


//
// get location info from ip 
// returns true: call is success
//         false:     is not success
//
void get_location(LOCATION *loc) {
	char	buf[LINE_LEN];
	char	message[LINE_LEN];

	network_open(ip_url, OPEN_MODE_READ, OPEN_TRANS_NONE);
    err = network_json_parse(ip_url);
    handle_err("ip-api parse");

	network_json_query(ip_url, "/status", buf);
	if (strcmp(buf, "success") != 0) {
		network_json_query(ip_url, "/message", buf);
		network_close(ip_url);
		sprintf(message, "ip-api(%s)", buf);
		err = 0xff;					// set unknown error 
    	handle_err(message);
	}

	network_json_query(ip_url, "/city", loc->city);
	network_json_query(ip_url, "/countryCode", loc->countryCode);
	network_json_query(ip_url, "/lon", loc->lon);
	network_json_query(ip_url, "/lat", loc->lat);

	network_close(ip_url);
}
