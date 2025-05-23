#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "dbg.h"
#include "models.h"
#include "nmea.h"
#include "gnss_pump.h"
#include "gnss_ublox_ingress.h"
#include "mixer.h"
#include "trunner.h"

static volatile sig_atomic_t keep_running = 1;

static UBloxIngress *ingress;
static GNSSPump *pump;
static TRunner *pumpRunner;
static Mixer *mixer;
static TRunner *mixerRunner;

static char gga_msg_ok[] = "$GLGGA,172814.76,4717.112671,N,00833.914843,E,2,6,1.2,18.893,M,-25.669,M,2.0 0031*4F";
static char buf[100];

static int mocked_pump(char **out) {
	memset(buf, 0, 100);
	strncpy(buf, gga_msg_ok, strlen(gga_msg_ok));

	*out = buf;
	return 0;
}

static void output_handler(GNSS_Data data) {
	return;
}

static GNSS_Data gnssDataGetter();

static void sig_handler(int _)
{
	(void)_;
	keep_running = 0;
}

int main(int argc, char *argv[])
{
	UBloxIngressConfig ingressCfg = {
		.portname = "/dev/ttyACM0"
	};

	GNSSPumpConfig pumpCfg = {
		.readIntervalMs = MIN_GNSS_PUMPING_INTERVAL_MS,
		.parse = NMEA_parse,
		.pump = mocked_pump,
	};

	TRunnerConfig pumpRunnerCfg = {
		.name = "gnss-pump",
		.func = GNSSPump_pump,
	};

	MixerConfig mixerCfg = {
		.mixIntervalMs = MIN_MIXING_INTERVAL_MS,
		.getGnssData = gnssDataGetter,
		.validateGnssData = validate_gnss_data,
		.outputHandler = output_handler,
	};

	TRunnerConfig mixerRunnerCfg = {
		.name = "mixer",
		.func = Mixer_mix,
	};

	check(UBloxIngress_init(ingressCfg, &ingress) == 0, "could not init ingress");

	check(GNSSPump_init(pumpCfg, &pump) == 0, "could not init pump");
	check(TRunner_init(pumpRunnerCfg, &pumpRunner) == 0, "could not init pump runner");
	check(Mixer_init(mixerCfg, &mixer) == 0, "could not init mixer");
	check(TRunner_init(mixerRunnerCfg, &mixerRunner) == 0, "could not init mixer runner");

	log_info("seems everything is in place...");

	check(TRunner_start(pumpRunner, pump) == 0, "should be OK");
	check(TRunner_start(mixerRunner, mixer) == 0, "should be OK");

	log_info("running...");

	char *rawInput = NULL;

	signal(SIGINT, sig_handler);
	while (keep_running) {
		rawInput = UBloxIngress_read(ingress);
		if (rawInput != NULL) {
			printf("%s", rawInput);
			rawInput = NULL;
		}
	}

	log_info("got a ctrl+c...");

	TRunner_stop(mixerRunner);
	TRunner_destroy(mixerRunner);
	TRunner_stop(pumpRunner);
	TRunner_destroy(pumpRunner);

	Mixer_destroy(mixer);
	GNSSPump_destroy(pump);
	UBloxIngress_destroy(ingress);

	return 0;

error:
	TRunner_stop(mixerRunner);
	TRunner_destroy(mixerRunner);
	TRunner_stop(pumpRunner);
	TRunner_destroy(pumpRunner);

	Mixer_destroy(mixer);
	GNSSPump_destroy(pump);
	UBloxIngress_destroy(ingress);

	return 1;
}

static GNSS_Data gnssDataGetter() {
	return GNSSPump_get_current(pump);
}
