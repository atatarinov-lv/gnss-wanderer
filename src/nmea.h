#ifndef _nmea
#define _nmea

#include "models.h"

typedef enum NMEA_MessageCode {
	GLL = 1,
	GGA = 2,
	RMC = 3,
} NMEA_MessageCode;

#define MAX_WORDS 16

typedef struct Sentence {
	char *words[MAX_WORDS];
	int len;
} Sentence;

Sentence NMEA_create_sentence();

int NMEA_parse(char *in, GNSS_Data *out);
int NMEA_split_sentence(char *in, Sentence *out);

int NMEA_parse_system(char *word, GNSS_System *out);
int NMEA_get_message_code(char *word, NMEA_MessageCode *out);

int NMEA_parse_GLL(Sentence s, GNSS_Data *out);
int NMEA_parse_GGA(Sentence s, GNSS_Data *out);
int NMEA_parse_RMC(Sentence s, GNSS_Data *out);

int NMEA_parse_coordinate(char *word, float *out);
int NMEA_parse_time(char *word, float *out);
int NMEA_parse_knots(char *word, float *out);
int NMEA_parse_cource(char *word, float *out);

int NMEA_checksum(const char *command);

#endif
