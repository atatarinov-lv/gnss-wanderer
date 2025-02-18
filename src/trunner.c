#include <stdlib.h>

#include "dbg.h"
#include "trunner.h"

int TRunner_init(TRunnerConfig cfg, TRunner **out)
{
	TRunner *raw = NULL;

	check(cfg.func != NULL, "func must be set up");

	raw = calloc(1, sizeof(TRunner));
	check_mem(raw);

	raw->cfg = cfg;

	log_info("TRunner initialized: %s", cfg.name);

	*out = raw;

	return 0;

error:
	if (raw != NULL) {
		free(raw);
	}

	return 1;
}

int TRunner_destroy(TRunner *r)
{
	if (r == NULL) {
		return 0;
	}

	TRunnerConfig cfg = r->cfg;

	free(r);

	log_info("TRunner destroyed: %s", cfg.name);

	return 0;
}

int TRunner_start(TRunner *r, void *arg)
{
	check(r != NULL, "pump is not set up");
	check(r->thread == 0, "pumping thread already running");

	int rc = pthread_create(&r->thread, NULL, r->cfg.func, arg);
	check(rc == 0, "failed to create a thread: rc: %d", rc);

	return 0;

error:
	return 1;
}

int TRunner_stop(TRunner *r)
{
	check(r != NULL, "pump is not set up");
	check(r->thread != 0, "seems pumping thread is not running");
	check(pthread_cancel(r->thread) == 0, "failed to cancel pumping thread");

	r->thread = 0;

	return 0;

error:
	return 1;
}
