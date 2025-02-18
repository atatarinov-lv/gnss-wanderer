#include <string.h>
#include <sys/time.h>

#include "minunit.h"
#include "models.h"

char *test_validate_gnss_data() {
	GNSS_Data data;
	memset(&data, 0, sizeof(data));

	mu_assert(validate_gnss_data(data) == E_DATA_UNKNOWN_SYSTEM, "should be E_GR_UNKNOWN_SYSTEM");

	data.System = GPS;
	mu_assert(validate_gnss_data(data) == E_DATA_INVALID_TIME, "should be E_GR_INVALID_TIMESTAMP");

	data.UTCTimeMs = 1;
	mu_assert(validate_gnss_data(data) == E_DATA_INVALID_LONGITUDE, "should be E_GR_INVALID_LONGITUDE");

	data.Long = 1;
	mu_assert(validate_gnss_data(data) == E_DATA_INVALID_LATITUDE, "should be E_GR_INVALID_LATITUDE");

	data.Lat = 1;
	mu_assert(validate_gnss_data(data) == 0, "should be OK");

	return NULL;
}

char *test_get_unix_seconds_without_date()
{
	float out;

	struct timeval in = {
		.tv_sec = 1739694150,
		.tv_usec = 0,
	};

	mu_assert(get_unix_seconds_without_date(&in) == 30150.0, "should be 30150.0");

	in.tv_usec = 345678;
	float check = 30150.35;
	out = get_unix_seconds_without_date(&in);
	mu_assert(out == check, "30150.35 != %f", out);

	return NULL;
}

char *all_tests()
{
	mu_suite_start();

	mu_run_test(test_validate_gnss_data);
	mu_run_test(test_get_unix_seconds_without_date);

	return NULL;
}

RUN_TESTS(all_tests);
