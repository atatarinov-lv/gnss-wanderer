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

	log_info("TRunner: %s: initialized", cfg.name);

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

	log_info("TRunner: %s: destroyed", cfg.name);

	return 0;
}

int TRunner_start(TRunner *r, void *arg)
{
	if (r == NULL) {
		return 0;
	}

	check(r->thread == 0, "TRunner: %s: thread already running", r->cfg.name);

	int rc = pthread_create(&r->thread, NULL, r->cfg.func, arg);
	check(rc == 0, "TRunner: %s: failed to create a thread: rc: %d", r->cfg.name, rc);

	return 0;

error:
	return 1;
}

int TRunner_stop(TRunner *r)
{
	if (r == NULL) {
		return 0;
	}

	check(r->thread != 0, "TRunner: %s: thread is not running", r->cfg.name);
	check(pthread_cancel(r->thread) == 0, "TRunner: %s: failed to cancel thread", r->cfg.name);

	r->thread = 0;

	return 0;

error:
	return 1;
}
