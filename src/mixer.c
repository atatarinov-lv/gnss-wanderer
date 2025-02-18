#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "dbg.h"
#include "models.h"
#include "mixer.h"

static void default_output_handler(GNSS_Data data);

int Mixer_init(MixerConfig cfg, Mixer **out)
{
	Mixer *raw = NULL;

	check(cfg.mixIntervalMs >= MIN_MIXING_INTERVAL_MS, "mixIntervalMs must be >= %d", MIN_MIXING_INTERVAL_MS);
	check(cfg.getGnssData != NULL, "function getGnssData must be set up");
	check(cfg.validateGnssData != NULL, "function validateGnssData must be set up");

	if (cfg.outputHandler == NULL) {
		cfg.outputHandler = default_output_handler;
	}

	raw = calloc(1, sizeof(Mixer));
	check_mem(raw);

	raw->cfg = cfg;
	raw->maxGnssDelay = (float)cfg.mixIntervalMs / 1000;

	log_info("Mixer initialized");

	*out = raw;

	return 0;

error:
	if (raw != NULL) {
		free(raw);
	}

	return 1;
}

int Mixer_destroy(Mixer *m)
{
	if (m == NULL) {
		return 0;
	}

	free(m);

	log_info("Mixer destroyed");

	return 0;
}

void* Mixer_mix(void* data) {
	Mixer *p = (Mixer*) data;

	GNSS_Data d;
	int rc;
	float currentUnixMs;

	while (1) {
		usleep(p->cfg.mixIntervalMs * 1000);

		d = p->cfg.getGnssData();
		rc = p->cfg.validateGnssData(d);

		if (rc != 0) {
			log_warn("gnss data is invalid: %d", rc);
			continue;
		}

		currentUnixMs = get_unix_seconds_without_date(NULL);
		if (currentUnixMs == -1) {
			log_warn("faild to get current unix mS");
			continue;
		}

		if (currentUnixMs - d.UTCTimeMs > p->maxGnssDelay) {
			log_warn("too long GNSS time gap: currentUnixMs: %.2f, gnss_UTCTimeMs: %.2f", currentUnixMs, d.UTCTimeMs);
		}

		p->cfg.outputHandler(d);
	}
}

static void default_output_handler(GNSS_Data data) {
	printf(
		"%d|%.6f|%.6f|%.6f|%.6f\n",
			data.System, data.Lat, data.Long, data.Speed, data.Course
	);
};
