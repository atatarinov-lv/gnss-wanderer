#ifndef _models
#define _models

#include <time.h>
#include <sys/time.h>

typedef enum GNSS_System {
	COMBINATION = 1,
	GPS = 2,
	GLONASS = 3
} GNSS_System;

typedef struct GNSS_Data {
	GNSS_System System;
	float UTCTimeMs;
	float Lat;
	float Long;
	float Speed;
	float Course;
} GNSS_Data;

int validate_gnss_data(GNSS_Data data);

#define E_DATA_UNKNOWN_SYSTEM 1
#define E_DATA_INVALID_TIME 2
#define E_DATA_INVALID_LONGITUDE 3
#define E_DATA_INVALID_LATITUDE 4

float get_unix_seconds_without_date(struct timeval *toConvert);

#endif
