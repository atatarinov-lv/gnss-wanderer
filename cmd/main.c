#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dbg.h"
#include "models.h"
#include "nmea.h"
#include "gnss_pump.h"
#include "ublox_ingress.h"
#include "mixer.h"
#include "trunner.h"

char* find_gnss_device();

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
	char *device_name = find_gnss_device();

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

#if defined(_UBLOX8) && defined(_LINUX)

#include <libudev.h>

#define UBLOX_VENDOR_ID "1546"

char* find_gnss_device() {
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

#elif defined(_UBLOX8) && defined(_MACOS)

#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <CoreFoundation/CoreFoundation.h>

#define UBLOX_VENDOR_ID 0x1546

char* find_gnss_device() {
	char *device_name = NULL;
	io_iterator_t iterator = 0;
	io_service_t service = 0;
	CFMutableDictionaryRef matching_dict = NULL;

	// 1. Create a dictionary to match all serial ports
	matching_dict = IOServiceMatching(kIOSerialBSDServiceValue);
	if (!matching_dict) {
		log_err("failed to create matching dictionary");
		return NULL;
	}

	// 2. Get an iterator for all serial services
	// kIOMainPortDefault is required for macOS 12+ (Fresh SDK)
	kern_return_t kr = IOServiceGetMatchingServices(kIOMainPortDefault, matching_dict, &iterator);

	// IOServiceGetMatchingServices consumes the reference to matching_dict on success.
	// We only release it if the call failed.
	if (kr != KERN_SUCCESS) {
		log_err("failed to get matching services");
		CFRelease(matching_dict);
		return NULL;
	}

	// 3. Iterate over all serial ports
	while ((service = IOIteratorNext(iterator))) {
		io_registry_entry_t parent = 0;
		io_registry_entry_t child = service;
		bool is_ublox = false;

		// 4. Walk up the registry tree to find the USB parent
		while (IORegistryEntryGetParentEntry(child, kIOServicePlane, &parent) == KERN_SUCCESS) {
			CFTypeRef vendor_id_ref = IORegistryEntryCreateCFProperty(
				parent,
				CFSTR("idVendor"),
				kCFAllocatorDefault,
				0
			);

			if (vendor_id_ref) {
				// idVendor is provided as a CFNumber on macOS
				if (CFGetTypeID(vendor_id_ref) == CFNumberGetTypeID()) {
					int32_t vendor_id = 0;
					CFNumberGetValue((CFNumberRef)vendor_id_ref, kCFNumberSInt32Type, &vendor_id);

					if (vendor_id == UBLOX_VENDOR_ID) {
						is_ublox = true;
					}
				}
				CFRelease(vendor_id_ref);
			}

			// Release the child node we just traversed from.
			// We do NOT release 'service' here because the outer loop handles it.
			if (child != service) {
				IOObjectRelease(child);
			}

			if (is_ublox) {
				// Found the device. Release the current 'parent' reference
				// because we are breaking out of the loop immediately.
				IOObjectRelease(parent);
				break;
			}

			// Move up the tree
			child = parent;
		}

		// If we exited the loop naturally (no break), 'child' holds the topmost parent reached.
		// Release it if it isn't the original service.
		if (!is_ublox && child != service) {
			IOObjectRelease(child);
		}

		if (is_ublox) {
			// 5. Get the device path (IOCalloutDevice is typically /dev/cu.usbmodem...)
			CFTypeRef dev_path_ref = IORegistryEntryCreateCFProperty(
				service,
				CFSTR(kIOCalloutDeviceKey),
				kCFAllocatorDefault,
				0
			);

			if (dev_path_ref && CFGetTypeID(dev_path_ref) == CFStringGetTypeID()) {
				char path_buffer[1024];
				if (CFStringGetCString((CFStringRef)dev_path_ref, path_buffer, sizeof(path_buffer), kCFStringEncodingUTF8)) {
					debug("first ublox device: %s", path_buffer);

					size_t len = strlen(path_buffer);
					device_name = calloc(len + 1, 1);
					check_mem(device_name);
					memcpy(device_name, path_buffer, len);
				}
			}

			if (dev_path_ref) CFRelease(dev_path_ref);
		}

		// Outer loop releases the current serial service
		IOObjectRelease(service);

		if (device_name != NULL) {
			debug("device is found");
			break;
		}

		continue;

		error:
			break;
	}

	IOObjectRelease(iterator);
	// matching_dict was consumed by IOServiceGetMatchingServices, do not release here.

	return device_name;
}

#endif
