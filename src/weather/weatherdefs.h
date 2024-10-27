#ifndef	WEATHERDEFS_H
#define WEATHERDEFS_H
#define	LINE_LEN	40
#define	HALF_LEN	20
#define	QUARTER_LEN	10

extern char large_buf[1024];

struct location_info {
	char	lon[HALF_LEN];
	char	lat[HALF_LEN];
	char	city[HALF_LEN];
	char	countryCode[QUARTER_LEN];
};
typedef struct location_info LOCATION;

struct weather_info {
	char	name[HALF_LEN];
	char	country[HALF_LEN];
	char	timezone[LINE_LEN];
	long	td;
	long	tz;						// time-zone offset
	char	icon;
	long	sunrise;	
	long	sunset;
	char	temp[QUARTER_LEN];	
	char	feels_like[QUARTER_LEN];
	char	pressure[QUARTER_LEN];
	char	humidity[QUARTER_LEN];
	char	dew_point[QUARTER_LEN];
	char	clouds[QUARTER_LEN];
	char	visibility[QUARTER_LEN];
	char	wind_speed[QUARTER_LEN];
	char	wind_deg[QUARTER_LEN];
};
typedef struct weather_info WEATHER;

struct day_info {
	long	td;
	long	sunrise;	
	long	sunset;
	char	temp_min[QUARTER_LEN];
	char	temp_max[QUARTER_LEN];
	char	icon;   
	char	precipitation_sum[QUARTER_LEN];
  	char	uv_index_max[QUARTER_LEN]; 
	char	wind_speed[QUARTER_LEN];
	char	wind_deg[QUARTER_LEN]; 
};
typedef struct day_info  DAYINFO;

struct forecast_info {
	DAYINFO day[8];
};
typedef struct forecast_info  FORECAST;
enum unit_option {
    METRIC,
    IMPERIAL
};
typedef enum unit_option UNITOPT;
#endif
