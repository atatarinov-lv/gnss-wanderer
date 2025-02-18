#ifndef _gnss_pump
#define _gnss_pump

#include "models.h"

#define MIN_GNSS_PUMPING_INTERVAL_MS 500

typedef struct GNSSPumpConfig {
	unsigned int readIntervalMs;
	int (*parse)(char *in, GNSS_Data *out);
	int (*pump)(char **out);
} GNSSPumpConfig;

typedef struct GNSSPump {
	GNSSPumpConfig cfg;
	_Atomic GNSS_Data currentData;
} GNSSPump;

int GNSSPump_init(GNSSPumpConfig cfg, GNSSPump **out);
int GNSSPump_destroy(GNSSPump *pump);
void* GNSSPump_pump(void* data);
GNSS_Data GNSSPump_get_current(GNSSPump *p);

#endif
