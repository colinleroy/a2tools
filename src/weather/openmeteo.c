/*
 *
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <conio.h>

#include "surl.h"
#include "weatherdefs.h"
#include "weatherui.h"
#include "weatherdisp.h"
#include "openmeteo.h"

extern UNITOPT unit_opt;
extern int	err;


char omurl[256];
char om_head[] = "https://api.open-meteo.com/v1/forecast?latitude=";
char om_lon[] = "&longitude=";

char om_tail_weather1[] ="&timezone=auto&timeformat=unixtime&current=relative_humidity_2m,weather_code,cloud_cover,surface_pressure";
char om_tail_weather2[] ="&current=temperature_2m,apparent_temperature,,wind_speed_10m,wind_direction_10m&hourly=dew_point_2m,visibility&forecast_hours=1";

char om_tail_forecast1[] ="&forecast_days=8&forecast_hours=1&timeformat=unixtime&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset";
char om_tail_forecast2[] ="&forecast_days=8&forecast_hours=1&daily=wind_speed_10m_max,wind_direction_10m_dominant,precipitation_sum,uv_index_max";

/* unit option string */
char *unit_str[] = {"&wind_speed_unit=ms", "&temperature_unit=fahrenheit&wind_speed_unit=mph"};

/* geocoding api */
char om_geocoding_head[] = "https://geocoding-api.open-meteo.com/v1/search?name=";
char om_geocoding_tail[] = "&count=1&language=en&format=json";

char city[40];

bool om_geocoding(LOCATION *loc, char *city) {
	strcpy(omurl, om_geocoding_head);
	strcat(omurl, city);
	strcat(omurl, om_geocoding_tail);

	surl_start_request(NULL, 0, omurl, SURL_METHOD_GET);
	err = !surl_response_ok();
	handle_err("open meteo geocoding parse");

	surl_get_json(loc->city, ".results[0].name", "ISO646-FR1", 0, HALF_LEN);
	if (strlen(loc->city) == 0) {
		return(false);
	}

	surl_get_json(loc->lon, ".results[0].lon", "ISO646-FR1", 0, HALF_LEN);
	surl_get_json(loc->lat, ".results[0].lat", "ISO646-FR1", 0, HALF_LEN);
	surl_get_json(loc->countryCode, ".results[0].country_code", "ISO646-FR1", 0, QUARTER_LEN);

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

	surl_start_request(NULL, 0, omurl, SURL_METHOD_GET);
	err = !surl_response_ok();
	handle_err("om parse");

//	city name, country code
	strcpy(wi->name, loc->city);
	strcpy(wi->country, loc->countryCode);

//  date & time
	surl_get_json(querybuf, ".current.time", "ISO646-FR1", 0, LINE_LEN);
	wi->td = atol(querybuf);
//  timezone(offset) 
	surl_get_json(querybuf, ".utc_offset_seconds", "ISO646-FR1", 0, LINE_LEN);
	wi->tz = atol(querybuf);
// timezone
	surl_get_json(wi->timezone, ".timezone", "ISO646-FR1", 0, LINE_LEN);
//  pressure
	surl_get_json(wi->pressure, ".current.surface_pressure", "ISO646-FR1", 0, QUARTER_LEN);
//  humidity
	surl_get_json(wi->humidity, ".current.relative_humidity_2m", "ISO646-FR1", 0, QUARTER_LEN);
// weather code (icon)
	surl_get_json(querybuf, ".current.weather_code", "ISO646-FR1", 0, LINE_LEN);
	wi->icon = atoi(querybuf);
//  clouds
	surl_get_json(wi->clouds, ".current.cloud_cover", "ISO646-FR1", 0, QUARTER_LEN);

// weather 2 query
	setup_omurl(loc, om_tail_weather2);

	progress_dots(1);

	surl_start_request(NULL, 0, omurl, SURL_METHOD_GET);
	err = !surl_response_ok();
	handle_err("omurl parse 2");

//  temperature
	surl_get_json(wi->temp, ".current.temperature_2m", "ISO646-FR1", 0, QUARTER_LEN);
//  feels_like
	surl_get_json(wi->feels_like, ".current.apparent_temperature", "ISO646-FR1", 0, QUARTER_LEN);
//  dew_point
	surl_get_json(wi->dew_point, ".hourly.dew_point_2m[0]", "ISO646-FR1", 0, QUARTER_LEN);
//  visibility
	surl_get_json(wi->visibility, ".hourly.visibility[0]", "ISO646-FR1", 0, QUARTER_LEN);
//  wind_speed
	surl_get_json(wi->wind_speed, ".current.wind_speed_10m", "ISO646-FR1", 0, QUARTER_LEN);
//  wind_deg
	surl_get_json(wi->wind_deg, ".current.wind_direction_10m", "ISO646-FR1", 0, QUARTER_LEN);

//	forecast
//  part 1
	setup_omurl(loc, om_tail_forecast1);

	progress_dots(2);

	surl_start_request(NULL, 0, omurl, SURL_METHOD_GET);
	err = !surl_response_ok();

	progress_dots(3);

	handle_err("forecast 1 parse");

	set_forecast1(fc);

//  copy today's sunrise/sunset from forecat data to weather data
	wi->sunrise = fc->day[0].sunrise; 
	wi->sunset = fc->day[0].sunset; 

//  part 2
	setup_omurl(loc, om_tail_forecast2);

	progress_dots(4);

	surl_start_request(NULL, 0, omurl, SURL_METHOD_GET);
	err = !surl_response_ok();
	handle_err("forecast 2 parse");

	progress_dots(5);

	set_forecast2(fc);

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
		sprintf(querybuf, ".daily.time[%d]", i);
		surl_get_json(prbuf, querybuf, "ISO646-FR1", 0, LINE_LEN);
		fc->day[i].td = atol(prbuf);
// sunrise 
		sprintf(querybuf, ".daily.sunrise[%d]", i);
		surl_get_json(prbuf, querybuf, "ISO646-FR1", 0, LINE_LEN);
		fc->day[i].sunrise = atol(prbuf);
// sunset 
		sprintf(querybuf, ".daily.sunset[%d]", i);
		surl_get_json(prbuf, querybuf, "ISO646-FR1", 0, LINE_LEN);
		fc->day[i].sunset = atol(prbuf);
// temp min
		sprintf(querybuf, ".daily.temperature_2m_min[%d]", i);
		surl_get_json(fc->day[i].temp_min, querybuf, "ISO646-FR1", 0, QUARTER_LEN);
// temp max
		sprintf(querybuf, ".daily.temperature_2m_max[%d]", i);
		surl_get_json(fc->day[i].temp_max, querybuf, "ISO646-FR1", 0, QUARTER_LEN);
// icon
		sprintf(querybuf, ".daily.weather_code[%d]", i);
		surl_get_json(prbuf, querybuf, "ISO646-FR1", 0, LINE_LEN);
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
		sprintf(querybuf, ".daily.precipitation_sum[%d]", i);
		surl_get_json(fc->day[i].precipitation_sum, querybuf, "ISO646-FR1", 0, QUARTER_LEN);
// uv index  max
		sprintf(querybuf, ".daily.uv_index_max[%d]", i);
		surl_get_json(fc->day[i].uv_index_max, querybuf, "ISO646-FR1", 0, QUARTER_LEN);
// wind  speed
		sprintf(querybuf, ".daily.wind_speed_10m_max[%d]", i);
		surl_get_json(fc->day[i].wind_speed, querybuf, "ISO646-FR1", 0, QUARTER_LEN);
// wind  deg
		sprintf(querybuf, ".daily.wind_direction_10m_dominant[%d]", i);
		surl_get_json(fc->day[i].wind_deg, querybuf, "ISO646-FR1", 0, QUARTER_LEN);
	}
}
