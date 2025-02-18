#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "dbg.h"
#include "gnss_ublox_ingress.h"

static int set_interface_attribs (int fd, int speed, int parity);
static int set_blocking (int fd, int should_block);
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

    // set speed B9600, 8n1 (no parity)
    check(set_interface_attribs(raw->fd, B9600, 0) == 0, "failed to set attribs");
    // log_info("attribs set up");

    // set no blocking
    // check(set_blocking(raw->fd, 0) == 0, "failed to set blocking mode");
    // log_info("blocking mode adjusted");

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

int UBloxIngress_write(UBloxIngress *ingress, char *command, int size) {
    check(ingress != NULL, "ingress is NULL");
    check(command != NULL, "command is NULL");
    check(size > 0, "size < 0");

    check(tcdrain(ingress->fd) >= 0, "failed to drain");

    check(write(ingress->fd, command, size) != -1, "failed to write");

    return 0;

error:
    return 1;
}

static int set_interface_attribs(int fd, int speed, int parity)
{
    struct termios tty;
    check(tcgetattr(fd, &tty) == 0, "error from tcgetattr");

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    // tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // // disable IGNBRK for mismatched speed tests; otherwise receive break
    // // as \000 chars
    // tty.c_iflag &= ~IGNBRK;         // disable break processing
    // tty.c_lflag = 0;                // no signaling chars, no echo,
    //                                 // no canonical processing
    // tty.c_oflag = 0;                // no remapping, no delays
    // tty.c_cc[VMIN]  = 0;            // read doesn't block
    // tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    // tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    // tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
    //                                 // enable reading

    // tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    // tty.c_cflag |= parity;
    // tty.c_cflag &= ~CSTOPB;
    // tty.c_cflag &= ~CRTSCTS;

    // check(tcsetattr(fd, TCSANOW, &tty) == 0, "error from tcsetattr");

    return 0;

error:
    return 1;
}

static int set_blocking(int fd, int should_block)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    check(tcgetattr(fd, &tty) == 0, "error from tcgetattr");

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    check(tcsetattr(fd, TCSANOW, &tty) == 0, "error from tcgetattr");

    return 0;

error:
    return 1;
}

static const char disableRMC[] = "PUBX,40,RMC,0,0,0,0,0,0";
static const char disableGLL[] = "PUBX,40,GLL,0,0,0,0,0,0";
static const char disableGSV[] = "PUBX,40,GSV,0,0,0,0,0,0";
static const char disableGSA[] = "PUBX,40,GSA,0,0,0,0,0,0";
static const char disableGGA[] = "PUBX,40,GGA,0,0,0,0,0,0";
static const char disableVTG[] = "PUBX,40,VTG,0,0,0,0,0,0";
static const char disableZDA[] = "PUBX,40,ZDA,0,0,0,0,0,0";

static int set_up_messages(UBloxIngress *ingress) {
    char *cmds[] = {
        "$PUBX,40,GLL,0,0,0,0,0,0*5C\r\n",
        // "$PUBX,40,TXT,0,0,0,0,0,0*43\r\n",
        // "$PUBX,40,GSA,0,0,0,0,0,0*4E\r\n",
        // "$PUBX,40,VTG,0,0,0,0,0,0*5E\r\n",
        // "$PUBX,40,GSV,0,0,0,0,0,0*59\r\n",
    };

    for (int i = 0; i < 1; i++) {
        char *cmd = cmds[i];
        log_info("command: %s", cmd);
        check(UBloxIngress_write(ingress, cmd, strlen(cmd)) == 0, "failed to write cmd: #%d", i);
    };

    return 0;

error:
    return 1;
}
