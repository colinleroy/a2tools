/*
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
#include "weatherui.h"
#include "weatherdisp.h"
#include "openmeteo.h"

extern UNITOPT unit_opt;
extern int	err;


char omurl[256];
char om_head[] = "N:https://api.open-meteo.com/v1/forecast?latitude=";
char om_lon[] = "&longitude=";

char om_tail_weather1[] ="&timezone=auto&timeformat=unixtime&current=relative_humidity_2m,weather_code,cloud_cover,surface_pressure";
char om_tail_weather2[] ="&current=temperature_2m,apparent_temperature,,wind_speed_10m,wind_direction_10m&hourly=dew_point_2m,visibility&forecast_hours=1";

char om_tail_forecast1[] ="&forecast_days=8&forecast_hours=1&timeformat=unixtime&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset";
char om_tail_forecast2[] ="&forecast_days=8&forecast_hours=1&daily=wind_speed_10m_max,wind_direction_10m_dominant,precipitation_sum,uv_index_max";

/* unit option string */
char *unit_str[] = {"&wind_speed_unit=ms", "&temperature_unit=fahrenheit&wind_speed_unit=mph"};

/* geocoding api */
char om_geocoding_head[] = "N:https://geocoding-api.open-meteo.com/v1/search?name=";
char om_geocoding_tail[] = "&count=1&language=en&format=json";

char city[40];

bool om_geocoding(LOCATION *loc, char *city) {
	strcpy(omurl, om_geocoding_head);
	strcat(omurl, city);
	strcat(omurl, om_geocoding_tail);

    network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
    err = network_json_parse(omurl);
    handle_err("open meteo geocoding parse");

    network_json_query(omurl, "/results/0/name", loc->city);
	if (strlen(loc->city) == 0) {
		return(false);
	}
    network_json_query(omurl, "/results/0/longitude", loc->lon);
    network_json_query(omurl, "/results/0/latitude", loc->lat);
    network_json_query(omurl, "/results/0/country_code", loc->countryCode);

	network_close(omurl);

	return(true);
}

//
// setup Open-Metro URL
//
void setup_omurl(LOCATION *loc, char *param) {
	strcpy(omurl, om_head);
    strcat(omurl, loc->lat);
    strcat(omurl, om_lon);
    strcat(omurl, loc->lon);
    strcat(omurl, param);
	strcat(omurl, unit_str[unit_opt]);
}
//
//
//
void get_om_info(LOCATION *loc, WEATHER *wi, FORECAST *fc) {
	char querybuf[LINE_LEN];

// weather 1 query
	setup_omurl(loc, om_tail_weather1);

	progress_dots(0);

    network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
    err = network_json_parse(omurl);
    handle_err("om parse");

//	city name, country code
	strcpy(wi->name, loc->city);
	strcpy(wi->country, loc->countryCode);

//  date & time
    network_json_query(omurl, "/current/time", querybuf);
	wi->td = atol(querybuf);
//  timezone(offset) 
    network_json_query(omurl, "/utc_offset_seconds", querybuf);
	wi->tz = atol(querybuf);
// timezone
    network_json_query(omurl, "/timezone", wi->timezone);
//  pressure
    network_json_query(omurl, "/current/surface_pressure", wi->pressure);
//  humidity
    network_json_query(omurl, "/current/relative_humidity_2m", wi->humidity);
// weather code (icon)
    network_json_query(omurl, "/current/weather_code", querybuf);
	wi->icon = atoi(querybuf);
//  clouds
    network_json_query(omurl, "/current/cloud_cover", wi->clouds);

	network_close(omurl);	// of weather1


// weather 2 query
	setup_omurl(loc, om_tail_weather2);

	progress_dots(1);

    network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
    err = network_json_parse(omurl);
    handle_err("omurl parse 2");

//  temperature
    network_json_query(omurl, "/current/temperature_2m", wi->temp);
//  feels_like
    network_json_query(omurl, "/current/apparent_temperature", wi->feels_like);
//  dew_point
    network_json_query(omurl, "/hourly/dew_point_2m/0", wi->dew_point);
//  visibility
    network_json_query(omurl, "/hourly/visibility/0", wi->visibility);
//  wind_speed
    network_json_query(omurl, "/current/wind_speed_10m", wi->wind_speed);
//  wind_deg
    err = network_json_query(omurl, "/current/wind_direction_10m", wi->wind_deg);

	network_close(omurl);	// of weather2

//	forecast
//  part 1
	setup_omurl(loc, om_tail_forecast1);

	progress_dots(2);

    network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);

	progress_dots(3);

    err = network_json_parse(omurl);
    handle_err("forecast 1 parse");

	set_forecast1(fc);
	network_close(omurl);	// of forecast part 1

//  copy today's sunrise/sunset from forecat data to weather data
	wi->sunrise = fc->day[0].sunrise; 
	wi->sunset = fc->day[0].sunset; 

//  part 2
	setup_omurl(loc, om_tail_forecast2);

	progress_dots(4);

    network_open(omurl, OPEN_MODE_READ, OPEN_TRANS_NONE);
    err = network_json_parse(omurl);
    handle_err("forecast 2 parse");

	progress_dots(5);

	set_forecast2(fc);

	network_close(omurl);	// of forecast part 2

	progress_dots(6);
}
//
// set forecast data part1
//
void set_forecast1(FORECAST *fc) {
	char	i;
	char querybuf[LINE_LEN];
	char prbuf[LINE_LEN];

	for (i=0; i<=7; i++) {
// date & time
		sprintf(querybuf, "/daily/time/%d", i);
	    network_json_query(omurl, querybuf, prbuf);
		fc->day[i].td = atol(prbuf);
// sunrise 
		sprintf(querybuf, "/daily/sunrise/%d", i);
	    network_json_query(omurl, querybuf, prbuf);
		fc->day[i].sunrise = atol(prbuf);
// sunset 
		sprintf(querybuf, "/daily/sunset/%d", i);
	    network_json_query(omurl, querybuf, prbuf);
		fc->day[i].sunset = atol(prbuf);
// temp min
		sprintf(querybuf, "/daily/temperature_2m_min/%d", i);
	    network_json_query(omurl, querybuf, fc->day[i].temp_min);
// temp max
		sprintf(querybuf, "/daily/temperature_2m_max/%d", i);
	    network_json_query(omurl, querybuf, fc->day[i].temp_max);
// icon
		sprintf(querybuf, "/daily/weather_code/%d", i);
	    network_json_query(omurl, querybuf, prbuf);
		fc->day[i].icon = atoi(prbuf);
	}
}
//
// set forecast data part 2
//
void set_forecast2(FORECAST *fc) {
	char	i;
	char querybuf[LINE_LEN];

	for (i=0; i<=7; i++) {
// precipitation sum
		sprintf(querybuf, "/daily/precipitation_sum/%d", i);
	    network_json_query(omurl, querybuf, fc->day[i].precipitation_sum);
// uv index  max
		sprintf(querybuf, "/daily/uv_index_max/%d", i);
	    network_json_query(omurl, querybuf, fc->day[i].uv_index_max);
// wind  speed
		sprintf(querybuf, "/daily/wind_speed_10m_max/%d", i);
	    network_json_query(omurl, querybuf, fc->day[i].wind_speed);
// wind  deg
		sprintf(querybuf, "/daily/wind_direction_10m_dominant/%d", i);
	    network_json_query(omurl, querybuf, fc->day[i].wind_deg);
	}
}
