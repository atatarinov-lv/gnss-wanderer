#include <unistd.h>

#include "minunit.h"
#include "gnss_pump.h"
#include "trunner.h"

static GNSSPump *pump;
static TRunner *runner;

static unsigned int readIntervalMs = 999;

static int counter = 0;
static int stop = 1;

static int fake_parse (char *in, GNSS_Data *out)
{
	debug("a good mocked function parse");
	counter++;

	out->Lat = (float)counter;
	out->Long = (float)counter;

	if (counter >= 5) {
		stop = 0;
	} else {
		sleep(1);
	}
	return 0;
}

static int fake_pump (char **out)
{
	debug("a good mocked function pump");
	return 0;
}

char *test_init()
{
	GNSSPumpConfig cfg = {
		.readIntervalMs = MIN_GNSS_PUMPING_INTERVAL_MS - 1,
		.parse = NULL,
		.pump = NULL,
	};

	mu_assert(GNSSPump_init(cfg, &pump) == 1, "invalid interval");

	cfg.readIntervalMs = readIntervalMs;
	mu_assert(GNSSPump_init(cfg, &pump) == 1, "not set up function parse");

	cfg.parse = fake_parse;
	mu_assert(GNSSPump_init(cfg, &pump) == 1, "not set up function pump");

	cfg.pump = fake_pump;
	mu_assert(GNSSPump_init(cfg, &pump) == 0, "should be OK");

	TRunnerConfig runnerCfg = {
		.name = "gnss-pump",
		.func = GNSSPump_pump,
	};

	mu_assert(TRunner_init(runnerCfg, &runner) == 0, "should be OK");

	return NULL;
}

char *test_destroy()
{
	mu_assert(GNSSPump_destroy(pump) == 0, "destroy failed");

	mu_assert(GNSSPump_destroy(NULL) == 0, "nothing to destroy");

	mu_assert(TRunner_destroy(runner) == 0, "should be OK");

	return NULL;
}

char *test_cfg_fields()
{
	GNSSPumpConfig cfg = pump->cfg;

	mu_assert(cfg.readIntervalMs == readIntervalMs, "%d != %d", cfg.readIntervalMs, readIntervalMs);
	mu_assert(cfg.parse == fake_parse, "%p != %p", cfg.parse, fake_parse);

	return NULL;
}

char *test_pump()
{
	mu_assert(TRunner_start(runner, pump) == 0, "should be OK");

	while (stop != 0) {
		GNSS_Data data = GNSSPump_get_current(pump);
		debug("lat: %f", data.Lat);
		debug("long: %f", data.Long);

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
	mu_run_test(test_pump);
#endif

	mu_run_test(test_destroy);

	return NULL;
}

RUN_TESTS(all_tests);
