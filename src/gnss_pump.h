#ifndef _gnss_pump
#define _gnss_pump

#include <pthread.h>

#include "models.h"

#define MIN_GNSS_PUMPING_INTERVAL_MS 100

typedef struct GNSSPump_config {
    unsigned int readIntervalMs;
    int (*parse)(char *in, GNSS_Data *out);
    int (*pump)(char *out);
} GNSSPump_config;

typedef struct GNSSPump {
    GNSSPump_config cfg;
    pthread_t thread;
    _Atomic GNSS_Data currentData;
} GNSSPump;

int GNSSPump_init(GNSSPump_config cfg, GNSSPump **out);
int GNSSPump_destroy(GNSSPump *pump);
int GNSSPump_start(GNSSPump *pump);
int GNSSPump_stop(GNSSPump *pump);

#endif
