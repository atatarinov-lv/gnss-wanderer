#include <unistd.h>

#include "dbg.h"
#include "minunit.h"
#include "mixer.h"
#include "models.h"
#include "trunner.h"

static Mixer *mixer;
static TRunner *runner;

static unsigned int mixIntervalMs = 999;

static int counter = -1;
static int stop = 1;

static GNSS_Data mocked_get_gnss_data() {
	counter++;

	float UTCTimeMs = counter;

	if (counter % 4 == 0) {
		UTCTimeMs = get_unix_seconds_without_date(NULL);
	}

	GNSS_Data out = {
		.Lat = counter,
		.Long = counter,
		.Speed = counter,
		.Course = counter,
		.UTCTimeMs = UTCTimeMs,
		.System = counter,
	};

	return out;
}

static int mocker_validate_gnss_data(GNSS_Data data) {
	if (counter > 10) {
		stop = 0;
	}

	return counter % 2;
}

static void mocked_output_handler(GNSS_Data data) {
	debug(
		"%d|%.6f|%.6f|%.6f|%.6f\n",
		data.System, data.Lat, data.Long, data.Speed, data.Course
	);
}

char *test_init()
{
	MixerConfig cfg = {
		.mixIntervalMs = MIN_MIXING_INTERVAL_MS - 1,
		.getGnssData = NULL,
		.validateGnssData = NULL,
		.outputHandler = mocked_output_handler,
	};

	mu_assert(Mixer_init(cfg, &mixer) == 1, "invalid interval");

	cfg.mixIntervalMs = mixIntervalMs;
	mu_assert(Mixer_init(cfg, &mixer) == 1, "not set up function getGnssData");

	cfg.getGnssData = mocked_get_gnss_data;
	mu_assert(Mixer_init(cfg, &mixer) == 1, "not set up function validateGnssData");

	cfg.validateGnssData = mocker_validate_gnss_data;
	mu_assert(Mixer_init(cfg, &mixer) == 0, "should be OK");

	TRunnerConfig runnerCfg = {
		.name = "mixer",
		.func = Mixer_mix,
	};

	mu_assert(TRunner_init(runnerCfg, &runner) == 0, "should be OK");

	return NULL;
}

char *test_destroy()
{
	mu_assert(Mixer_destroy(mixer) == 0, "destroy failed");

	mu_assert(Mixer_destroy(NULL) == 0, "nothing to destroy");

	mu_assert(TRunner_destroy(runner) == 0, "should be OK");

	return NULL;
}

char *test_cfg_fields()
{
	MixerConfig cfg = mixer->cfg;

	mu_assert(cfg.mixIntervalMs == mixIntervalMs, "%d != %d", cfg.mixIntervalMs, mixIntervalMs);
	mu_assert(cfg.validateGnssData == mocker_validate_gnss_data, "%p != %p", cfg.validateGnssData, mocker_validate_gnss_data);
	mu_assert(cfg.getGnssData == mocked_get_gnss_data, "%p != %p", cfg.getGnssData, mocked_get_gnss_data);

	return NULL;
}

char *test_mix()
{
	mu_assert(TRunner_start(runner, mixer) == 0, "should be OK");

	while (stop != 0) {
		usleep(500000);
	}

	mu_assert(TRunner_stop(runner) == 0, "should be OK");

	return NULL;
}

char *all_tests()
{
	mu_suite_start();

	mu_run_test(test_init);
	mu_run_test(test_cfg_fields);

#ifndef _DISABLE_LONG_TEST
	mu_run_test(test_mix);
#endif

	mu_run_test(test_destroy);

	return NULL;
}

RUN_TESTS(all_tests);
