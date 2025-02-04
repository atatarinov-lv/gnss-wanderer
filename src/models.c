#include "dbg.h"
#include "models.h"

int validate_gnss_data(GNSS_Data data) {
    if (data.System < 1) {
        log_err("unknown system: %d", data.System);
        return E_DATA_UNKNOWN_SYSTEM;
    }

    if (data.UTCTimeMs <= 0) {
        log_err("invalid timestamp: %f", data.UTCTimeMs);
        return E_DATA_INVALID_TIME;
    }

    if (data.Long <= 0) {
        log_err("invalid long: %f", data.Long);
        return E_DATA_INVALID_LONGITUDE;
    }

    if (data.Lat <= 0) {
        log_err("invalid lat: %f", data.Lat);
        return E_DATA_INVALID_LATITUDE;
    }

    return 0;
}
