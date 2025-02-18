#ifndef _gnss_blox_ingress
#define _gnss_blox_ingress

#define MIN_GNSS_READER_BUF_SIZE 256
#define MAX_GNSS_READER_BUF_SIZE 1024

typedef struct UBloxIngressConfig {
	char *portname;
} UBloxIngressConfig;


typedef struct UBloxIngress {
	UBloxIngressConfig cfg;
	int fd;
	char buf [MAX_GNSS_READER_BUF_SIZE];
} UBloxIngress;

int UBloxIngress_init(UBloxIngressConfig cfg, UBloxIngress **out);
int UBloxIngress_destroy(UBloxIngress *out);
char* UBloxIngress_read(UBloxIngress *ingress);
int UBloxIngress_write(UBloxIngress *ingress, const char *command);

#endif
