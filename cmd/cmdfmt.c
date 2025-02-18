#include "stdio.h"
#include "string.h"

#include "dbg.h"
#include "nmea.h"
#include "ublox_msg.h"

const char* substitute_cmd(char *input);

int main(int argc, char *argv[]) {
	debug("argc: %d", argc);
	check(argc > 1, "no commands at the input");

	int sum;
	const char* cmd;

	for (int i = 1; i < argc; i++) {
		cmd = substitute_cmd(argv[i]);

		log_info("argv[%d]: %s", i, cmd);

		sum = NMEA_checksum(cmd);

		printf("$%s*%X\r\n", cmd, sum);

		log_info("command printed");
	}

	return 0;

error:
	return 1;
}

const char* substitute_cmd(char *input) {
	if (strcmp(input, "-gsv") == 0) {
		return ublox_disable_GSV;
	} else if (strcmp(input, "-gsa") == 0) {
		return ublox_disable_GSA;
	} else if (strcmp(input, "-vtg") == 0) {
		return ublox_disable_VTG;
	} else if (strcmp(input, "-zda") == 0) {
		return ublox_disable_ZDA;
	}

	return input;
}
