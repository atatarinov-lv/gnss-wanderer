#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "dbg.h"
#include "mixer.h"

int Mixer_init(MixerConfig cfg, Mixer **out)
{
    Mixer *raw = NULL;

    check(cfg.mixIntervalMs >= MIN_MIXING_INTERVAL_MS, "mixIntervalMs must be >= %d", MIN_MIXING_INTERVAL_MS);
    check(cfg.getGnssData != NULL, "function getGnssData must be set up");
    check(cfg.validateGnssData != NULL, "function validateGnssData must be set up");

    raw = calloc(1, sizeof(Mixer));
    check_mem(raw);

    raw->cfg = cfg;

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

    while (1) {
        usleep(p->cfg.mixIntervalMs * 1000);

        d = p->cfg.getGnssData();
        rc = p->cfg.validateGnssData(d);

        if (rc != 0) {
            log_warn("gnss data is invalid: %d", rc);
            continue;
        }

        printf(
            "%d|%.6f|%.6f|%.6f|%.6f\n",
            d.System, d.Lat, d.Long, d.Speed, d.Course
        );
    }
}
