#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libudev.h>

#include "dbg.h"
#include "models.h"
#include "nmea.h"
#include "gnss_pump.h"
#include "ublox_ingress.h"
#include "mixer.h"
#include "trunner.h"

char* find_gnss_device_with_udev();

static volatile sig_atomic_t keep_running = 1;

static UBloxIngress *ingress;
static GNSSPump *pump;
static TRunner *pumpRunner;
static Mixer *mixer;
static TRunner *mixerRunner;

static int mocked_pump(char **out) {
	*out = UBloxIngress_read(ingress);

	for (char *symb = *out; *symb != '\0'; symb++) {
		if (*symb == '\r' || *symb == '\n') {
			*symb = '\0';
			break;
		}
	}

	debug("%s", *out);
	return 0;
}

static void output_handler(GNSS_Data data) {
	printf(
		"%d|%.6f|%.6f|%.6f|%.6f\n",
			data.System, data.Lat, data.Long, data.Speed, data.Course
	);
};

static GNSS_Data gnssDataGetter() {
	return GNSSPump_get_current(pump);
}

static void sig_handler(int _)
{
	(void)_;
	keep_running = 0;
}

int main(int argc, char *argv[])
{
	char *device_name = find_gnss_device_with_udev();
	
	check(device_name != NULL, "can not detect GNSS device");
	log_info("gonna use device: %s", device_name);
	
	UBloxIngressConfig ingressCfg = {
		.portname = device_name
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
		sleep(1);
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


#ifdef UBLOX8

#define UBLOX_VENDOR_ID "1546"

char* find_gnss_device_with_udev() {
	char *device_name = NULL;

	struct udev *udev = udev_new();
	if (!udev) {
		log_err("failed to create udev context");
		return NULL;
	}

	struct udev_enumerate *enumerate = udev_enumerate_new(udev);
	if (!enumerate) {
		log_err("failed to create enumerate context");
		udev_unref(udev);
		return NULL;
	}

	udev_enumerate_add_match_subsystem(enumerate, "tty");
	udev_enumerate_scan_devices(enumerate);

	struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
	struct udev_list_entry *entry;

	udev_list_entry_foreach(entry, devices) {
		const char *syspath = udev_list_entry_get_name(entry);
		struct udev_device *tty_dev = udev_device_new_from_syspath(udev, syspath);
		if (!tty_dev) continue;

		struct udev_device *usb_parent = udev_device_get_parent_with_subsystem_devtype(tty_dev, "usb", "usb_device");
		if (usb_parent) {
			const char *vendor_id = udev_device_get_sysattr_value(usb_parent, "idVendor");
			
			if (vendor_id && strcasecmp(vendor_id, UBLOX_VENDOR_ID) == 0) {
				const char *devnode = udev_device_get_devnode(tty_dev);

				if (devnode) {
					debug("first ublox device: %s", devnode);
					int dev_node_len = strlen(devnode);
					device_name = calloc(dev_node_len + 1, 1);
					check_mem(device_name);
					memcpy(device_name, devnode, dev_node_len);
				}
			}
		}

		udev_device_unref(tty_dev);

		if (device_name != NULL) {
			debug("device is found");
			goto found;
		}

		continue;

		error:
			udev_device_unref(tty_dev);
			break;
	}

found:
	udev_enumerate_unref(enumerate);
	udev_unref(udev);

	return device_name;
}

#endif
