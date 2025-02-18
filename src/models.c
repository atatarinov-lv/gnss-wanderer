#include <math.h>
#include <time.h>
#include <sys/time.h>

#include "dbg.h"
#include "models.h"

int validate_gnss_data(GNSS_Data data) {
	if (data.System < 1) {
		log_err("unknown system: %d", data.System);
		return E_DATA_UNKNOWN_SYSTEM;
	}

	if (data.UTCTimeMs <= 0) {
		log_err("invalid timestamp: %f", data.UTCTimeMs);
		return E_DATA_INVALID_TIME;
	}

	if (data.Long <= 0) {
		log_err("invalid long: %f", data.Long);
		return E_DATA_INVALID_LONGITUDE;
	}

	if (data.Lat <= 0) {
		log_err("invalid lat: %f", data.Lat);
		return E_DATA_INVALID_LATITUDE;
	}

	return 0;
}

float get_unix_seconds_without_date(struct timeval *toConvert) {

	struct timeval tv;

	if (toConvert != NULL) {
		tv = *toConvert;
	} else {
		check(gettimeofday(&tv, NULL) == 0, "failed to get the time since epoch");
	}

	debugVV("sec: %ld, usec: %ld", tv.tv_sec, tv.tv_usec);

	struct tm parsed;

	check(gmtime_r(&tv.tv_sec, &parsed) != NULL, "failed to parse the time");

	debugVV("hours: %d, minutes: %d, seconds: %d", parsed.tm_hour, parsed.tm_min, parsed.tm_sec);

	float out = parsed.tm_hour * 60 * 60 + parsed.tm_min * 60 + parsed.tm_sec;

	float n100th = roundf((float)tv.tv_usec / 10000) / 100;

	return out + n100th;

error:
	return -1;
}
