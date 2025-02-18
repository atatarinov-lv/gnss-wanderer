#include <math.h>
#include <stdlib.h>

#include "dbg.h"
#include "models.h"
#include "nmea.h"

#define GGL_ID "GLL"
#define GGA_ID "GGA"
#define RMC_ID "RMC"

#define GPS_ID "GP"
#define GLONASS_ID "GL"
#define GNSS_COMBINATION_ID "GN"

#define kmPERknot 1.852

int NMEA_parse(char *in, GNSS_Data *out) {
	check(in != NULL, "in is NULL");
	debugVV("input: \"%s\"", in);

	Sentence s = NMEA_create_sentence();

	check(NMEA_split_sentence(in, &s) == 0, "could not split sentence");
	check(s.len > 0, "no words at all");
	check(NMEA_parse_system(s.words[0], &out->System) == 0, "could not parse system");

	NMEA_MessageCode messageCode;
	check(NMEA_get_message_code(s.words[0], &messageCode) == 0, "could not get message code");

	switch (messageCode) {
	case GLL:
		check(NMEA_parse_GLL(s, out) == 0, "failed to parse GLL")
		break;
	case GGA:
		check(NMEA_parse_GGA(s, out) == 0, "failed to parse GGA")
		break;
	case RMC:
		check(NMEA_parse_RMC(s, out) == 0, "failed to parse RMC")
		break;
	default:
		sentinel("unknown message code");
	}

	return 0;

error:
	return 1;
}

int NMEA_split_sentence(char *in, Sentence *out) {
	check(in != NULL, "in is NULL");
	check(out != NULL, "out is NULL");

	if (strlen(in) == 0) {
		debugVV("in is empty");
		return 0;
	}

	debugVV("input: \"%s\"", in);

	char *word = NULL;

	for (unsigned int i = 0; (word = strsep(&in, ",")) && i < MAX_WORDS; i++) {
		debugVV("word: #%d: \"%s\"", i, word);
		out->words[i] = word;
		out->len++;
	}

	return 0;

error:
	return 1;
}

Sentence NMEA_create_sentence() {
	Sentence out;
	memset(&out, 0, sizeof(Sentence));
	return out;
}

int NMEA_parse_system(char *word, GNSS_System *out) {
	check(word != NULL, "word is NULL");
	check(out != NULL, "out is NULL");
	check(strlen(word) >= 3, "insufficient length: \"%s\"", word);
	check(word[0] == '$', "corrupted input: \"%s\"", word);

	debugVV("input: \"%s\"", word);

	if (strncmp(word + 1, GPS_ID, 2) == 0) {
		*out = GPS;
		return 0;
	} else if (strncmp(word + 1, GLONASS_ID, 2) == 0) {
		*out = GLONASS;
		return 0;
	}  else if (strncmp(word + 1, GNSS_COMBINATION_ID, 2) == 0) {
		*out = COMBINATION;
		return 0;
	}

error:
	return 1;
}

int NMEA_get_message_code(char *word, NMEA_MessageCode *out) {
	check(word != NULL, "word is NULL");
	check(out != NULL, "out is NULL");
	check(strlen(word) >= 6, "insufficient length: \"%s\"", word);
	check(word[0] == '$', "corrupted input: \"%s\"", word);

	debugVV("input: \"%s\"", word);

	if (strncmp(word + 3, GGL_ID, 2) == 0) {
		*out = GLL;
		return 0;
	} else if (strncmp(word + 3, GGA_ID, 2) == 0) {
		*out = GGA;
		return 0;
	}  else if (strncmp(word + 3, RMC_ID, 2) == 0) {
		*out = RMC;
		return 0;
	}

error:
	return 1;
}

int NMEA_parse_coordinate(char *word, float *out) {
	check(word != NULL, "word is NULL");
	check(out != NULL, "out is NULL");

	float raw = strtof(word, NULL);
	check(raw != 0, "invalid coordinate: \"%s\"", word)

	debugVV("parsed coordinate: %f", raw);

	float degree = (int)(raw / 100);
	debugVV("degrees: %f", degree);

	float minutes = raw - degree * 100;
	debugVV("minutes: %f", minutes);

	*out = degree + minutes / 60;
	debugVV("decimal degrees: %f", *out);

	return 0;

error:
	return 1;
}

int NMEA_parse_time(char *word, float *out) {
	check(word != NULL, "word is NULL");
	check(out != NULL, "out is NULL");

	debugVV("word: \"%s\"", word);

	float raw = strtof(word, NULL);
	check(raw != 0, "invalid time: \"%s\"", word);
	debugVV("parsed time: %.2f", raw);

	float hours = (int)(raw / 10000);
	debugVV("hours: %.0f", hours);

	float minutes = (int)(raw / 100 - hours * 100);
	debugVV("minutes: %.0f", minutes);

	float seconds = (int)(raw - hours * 10000 - minutes * 100);
	debugVV("seconds: %.0f", seconds);

	float miliseconds = roundf((raw - (int)raw) * 100) / 100;
	debugVV("miliseconds: %.2f", miliseconds);

	*out = hours * 60 * 60 + minutes * 60 + seconds + miliseconds;

	debugVV("seconds since 00-00-00: %.2f", *out);

	return 0;

error:
	return 1;
}

int NMEA_parse_knots(char *word, float *out) {
	check(word != NULL, "word is NULL");
	check(out != NULL, "out is NULL");

	debugVV("word: \"%s\"", word);

	float raw = strtof(word, NULL);
	debugVV("parsed knots: %.2f", raw);

	*out = raw * kmPERknot;

	return 0;

error:
	return 1;
}

int NMEA_parse_cource(char *word, float *out) {
	check(word != NULL, "word is NULL");
	check(out != NULL, "out is NULL");

	debugVV("word: \"%s\"", word);

	float raw = strtof(word, NULL);
	debugVV("parsed course: %.2f", raw);

	*out = raw;

	return 0;

error:
	return 1;
}


int NMEA_parse_GLL(Sentence s, GNSS_Data *out) {
	check(s.len >= 7, "not enought words: %d", s.len);
	check(out != NULL, "out is NULL");
	check(strlen(s.words[6]) == 1, "field status is too long: \"%s\"", s.words[6]);
	check(s.words[6][0] == 'A', "invalid status: \"%s\"", s.words[6])

	check(NMEA_parse_coordinate(s.words[1], &out->Lat) == 0, "could not parse latitude");
	check(NMEA_parse_coordinate(s.words[3], &out->Long) == 0, "could not parse longitude");
	check(NMEA_parse_time(s.words[5], &out->UTCTimeMs) == 0, "could not parse time");

	return 0;

error:
	return 1;
}

int NMEA_parse_GGA(Sentence s, GNSS_Data *out) {
	check(s.len >= 7, "not enought words: %d", s.len);
	check(out != NULL, "out is NULL");
	check(strlen(s.words[6]) == 1, "field quality is too long: \"%s\"", s.words[6]);
	check(s.words[6][0] == '1' || s.words[6][0] == '2', "invalid quality: \"%s\"", s.words[6])

	check(NMEA_parse_time(s.words[1], &out->UTCTimeMs) == 0, "could not parse time");
	check(NMEA_parse_coordinate(s.words[2], &out->Lat) == 0, "could not parse latitude");
	check(NMEA_parse_coordinate(s.words[4], &out->Long) == 0, "could not parse longitude");

	return 0;

error:
	return 1;
}

int NMEA_parse_RMC(Sentence s, GNSS_Data *out) {
	check(s.len >= 9, "not enought words: %d", s.len);
	check(out != NULL, "out is NULL");
	check(strlen(s.words[2]) == 1, "field status is too long: \"%s\"", s.words[2]);
	check(s.words[2][0] == 'A', "invalid status: \"%s\"", s.words[2])

	check(NMEA_parse_time(s.words[1], &out->UTCTimeMs) == 0, "could not parse time");
	check(NMEA_parse_coordinate(s.words[3], &out->Lat) == 0, "could not parse latitude");
	check(NMEA_parse_coordinate(s.words[5], &out->Long) == 0, "could not parse longitude");
	check(NMEA_parse_knots(s.words[7], &out->Speed) == 0, "could not parse speed");
	check(NMEA_parse_cource(s.words[8], &out->Course) == 0, "could not parse course");

	return 0;

error:
	return 1;
}

int NMEA_checksum(const char *command) {
	const char *symb;
	int sum;

	sum = 0;

	for (symb = command; *symb != '\0'; symb++) {
		sum  = sum ^ *symb;
	}

	return sum;
}