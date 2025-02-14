#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "minunit.h"
#include "gnss_pump.h"

GNSSPump *pump;

static unsigned int readIntervalMs = 999;

static int fake_parse (char *in, GNSS_Data *out)
{
    debug("a good mocked function parse");
    return 0;
}

static int fake_pump (char *out)
{
    debug("a good mocked function pump");
    return 0;
}

char *test_GNSSPump_init()
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

    return NULL;
}

char *test_GNSSPump_destroy()
{
    mu_assert(GNSSPump_destroy(pump) == 0, "destroy failed");

    mu_assert(GNSSPump_destroy(NULL) == 0, "nothing to destroy");

    return NULL;
}

char *test_cfg_fields()
{
    GNSSPumpConfig cfg = pump->cfg;

    mu_assert(cfg.readIntervalMs == readIntervalMs, "%d != %d", cfg.readIntervalMs, readIntervalMs);
    mu_assert(cfg.parse == fake_parse, "%p != %p", cfg.parse, fake_parse);

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_GNSSPump_init);
    mu_run_test(test_cfg_fields);
    // mu_run_test(test_GNSSPump_start);
    // sleep(5);
    // mu_run_test(test_GNSSPump_stop);
    mu_run_test(test_GNSSPump_destroy);

    return NULL;
}

RUN_TESTS(all_tests);
