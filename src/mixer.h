#ifndef _mixer
#define _mixer

#include "models.h"

#define MIN_MIXING_INTERVAL_MS 500

typedef struct MixerConfig {
	unsigned int mixIntervalMs;
	GNSS_Data (*getGnssData)();
	int (*validateGnssData)(GNSS_Data data);
	void (*outputHandler)(GNSS_Data data);
} MixerConfig;

typedef struct Mixer {
	MixerConfig cfg;
	float maxGnssDelay;
} Mixer;

int Mixer_init(MixerConfig cfg, Mixer **out);
int Mixer_destroy(Mixer *m);
void* Mixer_mix(void* data);

#endif
