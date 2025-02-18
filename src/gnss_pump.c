#include <stdlib.h>
#include <stdatomic.h>
#include <unistd.h>

#include "gnss_pump.h"
#include "dbg.h"
#include "models.h"

int GNSSPump_init(GNSSPumpConfig cfg, GNSSPump **out)
{
	GNSSPump *raw = NULL;

	check(cfg.readIntervalMs >= MIN_GNSS_PUMPING_INTERVAL_MS, "readIntervalMs must be >= %d", MIN_GNSS_PUMPING_INTERVAL_MS);

	check(cfg.parse != NULL, "function parse must be set up");
	check(cfg.pump != NULL, "function pump must be set up");

	raw = calloc(1, sizeof(GNSSPump));
	check_mem(raw);

	raw->cfg = cfg;

	log_info("GNSSPump initialized");

	*out = raw;

	return 0;

error:
	if (raw != NULL) {
		free(raw);
	}

	return 1;
}

int GNSSPump_destroy(GNSSPump *p)
{
	if (p == NULL) {
		return 0;
	}

	free(p);

	log_info("GNSSPump destroyed");

	return 0;
}

void* GNSSPump_pump(void* data) {
	GNSSPump *p = (GNSSPump*) data;

	char *rawData = NULL;
	GNSS_Data parsed;

	while (1) {
		usleep(p->cfg.readIntervalMs * 1000);

		if (p->cfg.pump(&rawData) != 0) {
			log_err("failed to pump data");
			continue;
		}

		if (p->cfg.parse(rawData, &parsed) != 0) {
			log_err("failed to parse data");
			continue;
		}

		atomic_store_explicit(&p->currentData, parsed, memory_order_relaxed);
	}
}

GNSS_Data GNSSPump_get_current(GNSSPump *p) {
	GNSS_Data tmp = atomic_load_explicit(&p->currentData, memory_order_relaxed);
	return tmp;
}
