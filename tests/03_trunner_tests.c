#include <unistd.h>

#include "dbg.h"
#include "minunit.h"
#include "trunner.h"

static TRunner *runner;
static char runnerName[] = "testing";
static int counter = 5;
static int stop = 1;

static void* mocked_func(void* arg)
{
	int *c = (int*) arg;

	for(int i = 0; i < *c; i++) {
		debug("a good mocked function: step: %d", i);
		sleep(1);
	}

	debug("a good mocked function: gonna send signal stop");

	stop = 0;

	return NULL;
}

char *test_init()
{
	TRunnerConfig cfg = {
		.name = runnerName,
		.func = NULL,
	};

	mu_assert(TRunner_init(cfg, &runner) == 1, "not set up func");

	cfg.func = mocked_func;
	mu_assert(TRunner_init(cfg, &runner) == 0, "should be OK");

	return NULL;
}

char *test_destroy()
{
	mu_assert(TRunner_destroy(runner) == 0, "destroy failed");

	mu_assert(TRunner_destroy(NULL) == 0, "nothing to destroy");

	return NULL;
}

char *test_cfg_fields()
{
	TRunnerConfig cfg = runner->cfg;

	mu_assert(cfg.func == mocked_func, "%p != %p", cfg.func, mocked_func);

	return NULL;
}

char *test_TRunner_start()
{
	mu_assert(TRunner_start(NULL, &counter) == 0, "NULL should be OK");
	mu_assert(TRunner_start(runner, &counter) == 0, "should be OK");
	mu_assert(runner->thread != 0, "should contain thread ID");
	mu_assert(TRunner_start(runner, &counter) == 1, "should fail: already running");

	return NULL;
}

char *test_TRunner_stop()
{
	mu_assert(TRunner_stop(NULL) == 0, "NULL should be OK");
	mu_assert(TRunner_stop(runner) == 0, "should be OK");
	mu_assert(runner->thread == 0, "should be 0");
	mu_assert(TRunner_stop(runner) == 1, "should fail: already stopped");

	return NULL;
}

char *all_tests()
{
	mu_suite_start();

	mu_run_test(test_init);
	mu_run_test(test_cfg_fields);

#ifndef _DISABLE_LONG_TEST
	mu_run_test(test_TRunner_start);
	while (stop != 0) {
		debug("waiting for signal stop");
		sleep(1);
	}
	mu_run_test(test_TRunner_stop);
#endif

	mu_run_test(test_destroy);

	return NULL;
}

RUN_TESTS(all_tests);
