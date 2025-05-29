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
#include "strsplit.h"

#include "weatherdefs.h"
#include "weatherui.h"
#include "weatherdisp.h"
#include "openmeteo.h"

extern UNITOPT unit_opt;
extern int	err;

char	n_lines;
char	**lines;

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

	surl_get_json(large_buf, ".results[0]|(.name,.longitude,.latitude,.country_code)",
								"US-ASCII", 0, sizeof(large_buf));
	
	n_lines = strsplit_in_place(large_buf, '\n', &lines);
	if (n_lines != 4) {
		err = 0xff;
		handle_err("Unexpected number of fields(geocoding)");
	}

	strncpy(loc->city, lines[0], HALF_LEN);
	if (strlen(loc->city) == 0) {
		return(false);
	}
	strncpy(loc->lon, lines[1], HALF_LEN);
	strncpy(loc->lat, lines[2], HALF_LEN);
	strncpy(loc->countryCode, lines[3], QUARTER_LEN);
	free(lines);

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
// weather 1 query
	setup_omurl(loc, om_tail_weather1);

	progress_dots(0);

	surl_start_request(NULL, 0, omurl, SURL_METHOD_GET);
	err = !surl_response_ok();
	handle_err("om parse");

//	city name, country code
	strcpy(wi->name, loc->city);
	strcpy(wi->country, loc->countryCode);

	surl_get_json(large_buf, ".current.time,.utc_offset_seconds,.timezone,"
													 ".current.surface_pressure,.current.relative_humidity_2m,"
													 ".current.weather_code,.current.cloud_cover",
								NULL, 0, sizeof(large_buf));
	
	n_lines = strsplit_in_place(large_buf, '\n', &lines);
	if (n_lines != 7) {
		err = 0xff;
		handle_err("Unexpected number of fields(om_info)");
	}

//  date & time
	wi->td = atol(lines[0]);
//  timezone(offset) 
	wi->tz = atol(lines[1]);
// timezone
	strncpy(wi->timezone, lines[2], LINE_LEN);
//  pressure
	strncpy(wi->pressure, lines[3], QUARTER_LEN);
//  humidity
	strncpy(wi->humidity, lines[4], QUARTER_LEN);
// weather code (icon)
	wi->icon = atoi(lines[5]);
//  clouds
	strncpy(wi->clouds, lines[6], QUARTER_LEN);

	free(lines);

// weather 2 query
	setup_omurl(loc, om_tail_weather2);

	progress_dots(1);

	surl_start_request(NULL, 0, omurl, SURL_METHOD_GET);
	err = !surl_response_ok();
	handle_err("omurl parse 2");

	surl_get_json(large_buf, ".current.temperature_2m,.current.apparent_temperature,"
													 ".hourly.dew_point_2m[0],.hourly.visibility[0],"
													 ".current.wind_speed_10m,.current.wind_direction_10m",
								NULL, 0, sizeof(large_buf));
	
	n_lines = strsplit_in_place(large_buf, '\n', &lines);
	if (n_lines != 6) {
		err = 0xff;
		handle_err("Unexpected number of fields(om_info_2)");
	}


//  temperature
	strncpy(wi->temp, lines[0], QUARTER_LEN);
//  feels_like
	strncpy(wi->feels_like, lines[1], QUARTER_LEN);
//  dew_point
	strncpy(wi->dew_point, lines[2], QUARTER_LEN);
//  visibility
	strncpy(wi->visibility, lines[3], QUARTER_LEN);
//  wind_speed
	strncpy(wi->wind_speed, lines[4], QUARTER_LEN);
//  wind_deg
	strncpy(wi->wind_deg, lines[5], QUARTER_LEN);

	free(lines);

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
	
	err = surl_get_json(large_buf, ".daily.time[],.daily.sunrise[],"
																 ".daily.sunset[],.daily.temperature_2m_min[],"
																 ".daily.temperature_2m_max[],.daily.weather_code[]",
											NULL, 0, sizeof(large_buf));
	if (err > 0) {
		err = 0;
	}
	handle_err("Wrong JSON answer\n");

	n_lines = strsplit_in_place(large_buf, '\n', &lines);
	if (n_lines % 8 != 0) {
		err = 0xff;
		handle_err("Unexpected number of fields(forecast1)");
	}

	for (i=0; i<=7; i++) {
// date & time
		fc->day[i].td = atol(lines[i]);
// sunrise 
		fc->day[i].sunrise = atol(lines[i+8]);
// sunset 
		fc->day[i].sunset = atol(lines[i+16]);
// temp min
		strncpy(fc->day[i].temp_min, lines[i+24], QUARTER_LEN);
// temp max
		strncpy(fc->day[i].temp_max, lines[i+32], QUARTER_LEN);
// icon
		fc->day[i].icon = atoi(lines[i+40]);
	}
	free(lines);
}
//
// set forecast data part 2
//
void set_forecast2(FORECAST *fc) {
	char	i;

	err = surl_get_json(large_buf, ".daily.precipitation_sum[],.daily.uv_index_max[],"
																 ".daily.wind_speed_10m_max[],.daily.wind_direction_10m_dominant[]",
											NULL, 0, sizeof(large_buf));
	if (err > 0) {
		err = 0;
	}
	handle_err("Wrong JSON answer\n");

	n_lines = strsplit_in_place(large_buf, '\n', &lines);
	if (n_lines % 8 != 0) {
		err = 0xff;
		handle_err("Unexpected number of fields(forecast2)");
	}

	for (i=0; i<=7; i++) {
// precipitation sum
		strncpy(fc->day[i].precipitation_sum, lines[i], QUARTER_LEN);
// uv index  max
		strncpy(fc->day[i].uv_index_max, lines[i+8], QUARTER_LEN);
// wind  speed
		strncpy(fc->day[i].wind_speed, lines[i+16], QUARTER_LEN);
// wind  deg
		strncpy(fc->day[i].wind_deg, lines[i+24], QUARTER_LEN);
	}
	free(lines);
}
