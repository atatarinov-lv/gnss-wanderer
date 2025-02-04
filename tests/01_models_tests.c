#include <string.h>
#include <stdio.h>

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

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_validate_gnss_data);

    return NULL;
}

RUN_TESTS(all_tests);
