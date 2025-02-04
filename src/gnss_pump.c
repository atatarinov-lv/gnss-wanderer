#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "gnss_pump.h"
#include "dbg.h"
#include "models.h"

static void* threadFunc(void* data);

int GNSSPump_init(GNSSPump_config cfg, GNSSPump **out)
{
    GNSSPump *raw = NULL;

    check(cfg.readIntervalMs >= MIN_GNSS_PUMPING_INTERVAL_MS, "readIntervalMs must be >= %d", MIN_GNSS_PUMPING_INTERVAL_MS);

    check(cfg.parse != NULL, "function parse must be set up");
    check(cfg.pump != NULL, "function pump must be set up");

    raw = calloc(1, sizeof(GNSSPump));
    check_mem(raw);

    raw->cfg = cfg;

    log_info("GNSS Pump initialized");

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

    log_info("GNSS Pump destroyed");

    return 0;
}

int GNSSPump_start(GNSSPump *p)
{
    check(p != NULL, "pump is not set up");
    check(p->thread == 0, "pumping thread already running");

    int rc = pthread_create(&p->thread, NULL, threadFunc, p);
    check(rc == 0, "failed to create a thread: rc: %d", rc);

    return 0;

error:
    return 1;
}

int GNSSPump_stop(GNSSPump *p)
{
    check(p != NULL, "pump is not set up");
    check(p->thread != 0, "seems pumping thread is not running");
    check(pthread_cancel(p->thread) == 0, "failed to cancel pumping thread");

    p->thread = 0;

    return 0;

error:
    return 1;
}

static void* threadFunc(void* data) {
    GNSSPump *p = (GNSSPump*) data;

    char *rawData = NULL;
    GNSS_Data parsed;

    while (1) {
        usleep(p->cfg.readIntervalMs * 1000);

        if (p->cfg.pump(rawData) != 0) {
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

// GNSS_Data GNSSPump_get_current_data() {
//     GNSS_Data tmp = atomic_load_explicit(&currentData, memory_order_relaxed);
//     return tmp;
// }

// int GNSSPump_set_current_data(GNSS_Data new) {
//     check(GNSSPump_validate_data(new) == 0, "new data is invalid");

//     atomic_store_explicit(&currentData, new, memory_order_relaxed);

//     return 0;

// error:
//     return 1;
// }
