#ifndef _mixer
#define _mixer

#include "models.h"

typedef struct MixerConfig {
    GNSS_Data (*get_GNSS_data)();
} MixerConfig;

typedef struct Mixer {
    MixerConfig cfg;
} Mixer;

#endif
