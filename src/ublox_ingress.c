#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "dbg.h"
#include "nmea.h"
#include "ublox_ingress.h"
#include "ublox_msg.h"

#define cmd_buf_len 128

static int set_interface_attribs (int fd, int speed);
static int set_up_messages(UBloxIngress *ingress);

int UBloxIngress_init(UBloxIngressConfig cfg, UBloxIngress **out) {
	UBloxIngress *raw = NULL;

	check(cfg.portname != NULL, "portname is NULL");

	raw = calloc(1, sizeof(UBloxIngress));
	check_mem(raw);
	memset(raw->buf, 0, sizeof(raw->buf));

	raw->fd = open(cfg.portname, O_RDWR | O_NOCTTY | O_SYNC);
	check(raw->fd > 0, "error opening: %s", cfg.portname);
	log_info("port opened");

	check(set_interface_attribs(raw->fd, B9600) == 0, "failed to set attribs");
	log_info("attribs set up");

	check(set_up_messages(raw) == 0, "failed to set up messages");
	log_info("messages set up");

	*out = raw;

	return 0;

error:
	if (raw != NULL) {
		free(raw);
	}

	return 1;
}

int UBloxIngress_destroy(UBloxIngress *out) {
	if (out == NULL) {
		return 0;
	}

	check(close(out->fd) == 0, "failed to close fd");
	log_info("port closed");

	free(out);
	log_info("ingress destroyed");

error:
	return 1;
}

char* UBloxIngress_read(UBloxIngress *ingress) {
	check(ingress != NULL, "ingress is NULL");

	int n = read(ingress->fd, ingress->buf, sizeof(ingress->buf) - 1);

	if (n > 1) {
		ingress->buf[n] = 0;
		return ingress->buf;
	}

error:
	return NULL;
}

int UBloxIngress_write(UBloxIngress *ingress, const char *cmd) {
	check(ingress != NULL, "ingress is NULL");
	check(cmd != NULL, "command is NULL");
	
	int size = strlen(cmd);
	check(size > 0, "size < 0");
	check(size < cmd_buf_len - 10 , "size > %d", cmd_buf_len - 10);

	char real_command[cmd_buf_len];

	sprintf(real_command, "$%s*%X\r\n", cmd, NMEA_checksum(cmd));

	check(write(ingress->fd, real_command, strlen(real_command)) != -1, "failed to write an ublox command");

	return 0;

error:
	return 1;
}

static int set_interface_attribs(int fd, int speed)
{
	struct termios tty;
	check(tcgetattr(fd, &tty) == 0, "error from tcgetattr");

	log_info("tty: initial c_iflag: %d", tty.c_iflag);
	tty.c_iflag &= ~IGNBRK;
	tty.c_iflag &= ~BRKINT;
	tty.c_iflag &= ~IGNPAR;
	tty.c_iflag &= ~PARMRK;
	tty.c_iflag &= ~INPCK;
	tty.c_iflag &= ~ISTRIP;
	tty.c_iflag &= ~INLCR;
	tty.c_iflag &= ~IGNCR;
	tty.c_iflag &= ~ICRNL;
	tty.c_iflag &= ~(IXON | IXOFF | IXANY);
	tty.c_iflag &= ~IUCLC;
	tty.c_iflag &= ~IMAXBEL;
	log_info("tty: tuned c_iflag: %d", tty.c_iflag);

	log_info("tty: initial c_oflag: %d", tty.c_oflag);
	tty.c_oflag &= ~OPOST;
	log_info("tty: tuned c_oflag: %d", tty.c_oflag);

	log_info("tty: initial c_lflag: %d", tty.c_lflag);
	tty.c_lflag &= ~ISIG;
	tty.c_lflag &= ~ICANON;
	log_info("tty: tuned c_lflag: %d", tty.c_lflag);

	cfsetospeed(&tty, speed);
	cfsetispeed(&tty, speed);

	log_info("tty speed set to %d", speed);

	check(tcsetattr(fd, TCSANOW, &tty) == 0, "error from tcsetattr");

	return 0;

error:
	return 1;
}

static int set_up_messages(UBloxIngress *ingress) {
	const char *cmds[] = {
		ublox_disable_GSA,
		ublox_disable_GSV,
		ublox_disable_VTG,
		ublox_disable_ZDA,
		ublox_disable_TXT,
		ublox_disable_GLL,
		ublox_disable_GGA,
	};

	for (int i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
		const char *cmd = cmds[i];
		log_info("command: %s", cmd);
		check(UBloxIngress_write(ingress, cmd) == 0, "failed to write cmd: #%s", cmds[i]);
	};

	return 0;

error:
	return 1;
}
