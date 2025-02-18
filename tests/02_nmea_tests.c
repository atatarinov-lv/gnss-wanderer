#include "minunit.h"
#include "models.h"
#include "nmea.h"
#include "ublox_msg.h"

char *test_NMEA_split_sentence()
{
	char test_EMPTY[] = "";
	char test_ONLY_DELIMITER[] = ",";
	char test_ZERO[] = "\0";
	char test_ONE_WORD[] = "word1";
	char test_TWO_WORDS[] = "word1,word2";
	char test_THREE_WORDS[] = ",word1,word2";
	char test_ALMOST_TRASH[] = ",,,word1,word2,,word3,,,,";

	Sentence out = NMEA_create_sentence();

	mu_assert(NMEA_split_sentence(NULL, &out) == 1, "in is NULL");
	mu_assert(NMEA_split_sentence(test_EMPTY, NULL) == 1, "out is NULL");

	out = NMEA_create_sentence();
	mu_assert(NMEA_split_sentence(test_EMPTY, &out) == 0, "should be OK");
	mu_assert(out.len == 0, "should be 0, but got %d", out.len);

	out = NMEA_create_sentence();
	mu_assert(NMEA_split_sentence(test_ONLY_DELIMITER, &out) == 0, "should be OK");
	mu_assert(out.len == 2, "should be 2, but got %d", out.len);

	out = NMEA_create_sentence();
	mu_assert(NMEA_split_sentence(test_ZERO, &out) == 0, "should be OK");
	mu_assert(out.len == 0, "should be 2, but got %d", out.len);

	out = NMEA_create_sentence();
	mu_assert(NMEA_split_sentence(test_ONE_WORD, &out) == 0, "should be OK");
	mu_assert(out.len == 1, "should be 0, but got %d", out.len);

	out = NMEA_create_sentence();
	mu_assert(NMEA_split_sentence(test_TWO_WORDS, &out) == 0, "should be OK");
	mu_assert(out.len == 2, "should be 2, but got %d", out.len);

	out = NMEA_create_sentence();
	mu_assert(NMEA_split_sentence(test_THREE_WORDS, &out) == 0, "should be OK");
	mu_assert(out.len == 3, "should be 3, but got %d", out.len);

	out = NMEA_create_sentence();
	mu_assert(NMEA_split_sentence(test_ALMOST_TRASH, &out) == 0, "should be OK");
	mu_assert(out.len == 11, "should be 11, but got %d", out.len);

	return NULL;
}

char *test_NMEA_parse_system()
{
	GNSS_System out;

	mu_assert(NMEA_parse_system(NULL, &out) != 0, "NULL")
	mu_assert(NMEA_parse_system("FAKE", NULL) != 0, "NULL")
	mu_assert(NMEA_parse_system("$F", &out) != 0, "insufficient length")
	mu_assert(NMEA_parse_system("GPRMC", &out) != 0, "corrupted input")

	mu_assert(NMEA_parse_system("$GPXXX", &out) == 0, "should be OK");
	mu_assert(out == GPS, "should be GPS");
	mu_assert(NMEA_parse_system("$GLXXX", &out) == 0, "should be OK");
	mu_assert(out == GLONASS, "should be GLONASS");
	mu_assert(NMEA_parse_system("$GNXXX", &out) == 0, "should be OK");
	mu_assert(out == COMBINATION, "should be COMBINATION");

	return NULL;
}

char *test_NMEA_get_message_code()
{
	NMEA_MessageCode out;

	mu_assert(NMEA_get_message_code(NULL, &out) != 0, "NULL")
	mu_assert(NMEA_get_message_code("FAKE", NULL) != 0, "NULL")
	mu_assert(NMEA_get_message_code("$F", &out) != 0, "insufficient length")
	mu_assert(NMEA_get_message_code("GPRMC", &out) != 0, "corrupted input")

	mu_assert(NMEA_get_message_code("$GPGLL", &out) == 0, "should be OK");
	mu_assert(out == GLL, "should be GLL");
	mu_assert(NMEA_get_message_code("$GLGGA", &out) == 0, "should be OK");
	mu_assert(out == GGA, "should be GGA");
	mu_assert(NMEA_get_message_code("$GNRMC", &out) == 0, "should be OK");
	mu_assert(out == RMC, "should be RMC");

	return NULL;
}

char *test_NMEA_parse_coordinate()
{
	float out;
	float check1 = 47.285213;
	float check2 = 8.565248;

	mu_assert(NMEA_parse_coordinate(NULL, &out) == 1, "NULL");
	mu_assert(NMEA_parse_coordinate("fake", NULL) == 1, "NULL");
	mu_assert(NMEA_parse_coordinate("not a number", &out) == 1, "should be error");
	mu_assert(NMEA_parse_coordinate("", &out) == 1, "should be error");

	mu_assert(NMEA_parse_coordinate("4717.112671", &out) == 0, "should be OK");
	mu_assert(out == check1, "should be %f", check1)

	mu_assert(NMEA_parse_coordinate("00833.914843", &out) == 0, "should be OK");
	mu_assert(out == check2, "should be %f", check2)

	return NULL;
}

char *test_NMEA_parse_time()
{
	float out;
	float check = 33801.47;

	mu_assert(NMEA_parse_time(NULL, &out) == 1, "NULL");
	mu_assert(NMEA_parse_time("fake", NULL) == 1, "NULL");
	mu_assert(NMEA_parse_time("not a number", &out) == 1, "should be error");
	mu_assert(NMEA_parse_time("", &out) == 1, "should be error");

	mu_assert(NMEA_parse_time("092321.47", &out) == 0, "should be OK");
	mu_assert(out == check, "should be %f", check);

	return NULL;
}

char *test_NMEA_parse_knots()
{
	float out;
	float check = 0.007408;

	mu_assert(NMEA_parse_knots(NULL, &out) == 1, "NULL");
	mu_assert(NMEA_parse_knots("fake", NULL) == 1, "NULL");

	mu_assert(NMEA_parse_knots("0.004", &out) == 0, "should be OK");
	mu_assert((int)(out * 1000000) == (int)(check * 1000000), "%f != %f", out, check);

	return NULL;
}

char *test_NMEA_parse_cource()
{
	float out;
	float check = 77.52;

	mu_assert(NMEA_parse_cource(NULL, &out) == 1, "NULL");
	mu_assert(NMEA_parse_cource("fake", NULL) == 1, "NULL");

	mu_assert(NMEA_parse_cource("77.52", &out) == 0, "should be OK");
	mu_assert((int)(out * 1000000) == (int)(check * 1000000), "%f != %f", out, check);

	return NULL;
}

char *test_NMEA_parse_GLL()
{
	Sentence s = NMEA_create_sentence();
	GNSS_Data out;

	float lat = 47.285213;
	float lon = 8.565248;
	float time = 33801.47;

	mu_assert(NMEA_parse_GLL(s, &out) == 1, "not enough words");

	s.len = 7;
	mu_assert(NMEA_parse_GLL(s, NULL) == 1, "out is NULL");

	s.words[6] = "too long status";
	mu_assert(NMEA_parse_GLL(s, &out) == 1, "too long status");

	s.words[6] = "V";
	mu_assert(NMEA_parse_GLL(s, &out) == 1, "invalid status");

	s.words[6] = "A";
	s.words[1] = "invalid latitude";
	mu_assert(NMEA_parse_GLL(s, &out) == 1, "invalid latitude");

	s.words[1] = "4717.112671";
	s.words[3] = "invalid longitude";
	mu_assert(NMEA_parse_GLL(s, &out) == 1, "invalid longitude");

	s.words[3] = "00833.914843";
	s.words[5] = "invalid time";
	mu_assert(NMEA_parse_GLL(s, &out) == 1, "invalid time");

	s.words[5] = "092321.47";
	mu_assert(NMEA_parse_GLL(s, &out) == 0, "should be OK");

	mu_assert(out.Lat == lat, "shoud be %f", lat);
	mu_assert(out.Long == lon, "shoud be %f", lon);
	mu_assert(out.UTCTimeMs == time, "shoud be %f", time);

	return NULL;
}

char *test_NMEA_parse_GGA()
{
	Sentence s = NMEA_create_sentence();
	GNSS_Data out;

	float lat = 47.285213;
	float lon = 8.565248;
	float time = 33801.47;

	mu_assert(NMEA_parse_GGA(s, &out) == 1, "not enough words");

	s.len = 7;
	mu_assert(NMEA_parse_GGA(s, NULL) == 1, "out is NULL");

	s.words[6] = "too long quality";
	mu_assert(NMEA_parse_GGA(s, &out) == 1, "too long quality");

	s.words[6] = "invalid quality";
	mu_assert(NMEA_parse_GGA(s, &out) == 1, "invalid quality");

	s.words[6] = "2";
	s.words[1] = "invalid time";
	mu_assert(NMEA_parse_GGA(s, &out) == 1, "invalid time");

	s.words[1] = "092321.47";
	s.words[2] = "invalid latitude";
	mu_assert(NMEA_parse_GGA(s, &out) == 1, "invalid latitude");

	s.words[2] = "4717.112671";
	s.words[4] = "invalid longitude";
	mu_assert(NMEA_parse_GGA(s, &out) == 1, "invalid longitude");

	s.words[4] = "00833.914843";
	mu_assert(NMEA_parse_GGA(s, &out) == 0, "should be OK");

	mu_assert(out.Lat == lat, "shoud be %f", lat);
	mu_assert(out.Long == lon, "shoud be %f", lon);
	mu_assert(out.UTCTimeMs == time, "shoud be %f", time);

	return NULL;
}

char *test_NMEA_parse_RMC()
{
	Sentence s = NMEA_create_sentence();
	GNSS_Data out;

	float lat = 47.285213;
	float lon = 8.565248;
	float time = 33801.47;
	float speed = 18.8904;
	float course = 77.52;

	mu_assert(NMEA_parse_RMC(s, &out) == 1, "not enough words");

	s.len = 9;
	mu_assert(NMEA_parse_RMC(s, NULL) == 1, "out is NULL");

	s.words[2] = "too long status";
	mu_assert(NMEA_parse_RMC(s, &out) == 1, "too long status");

	s.words[2] = "V";
	mu_assert(NMEA_parse_RMC(s, &out) == 1, "invalid status");

	s.words[2] = "A";
	s.words[1] = "invalid time";
	mu_assert(NMEA_parse_RMC(s, &out) == 1, "invalid time");

	s.words[1] = "092321.47";
	s.words[3] = "invalid latitude";
	mu_assert(NMEA_parse_RMC(s, &out) == 1, "invalid latitude");

	s.words[3] = "4717.112671";
	s.words[5] = "invalid longitude";
	mu_assert(NMEA_parse_RMC(s, &out) == 1, "invalid longitude");

	s.words[5] = "00833.914843";
	s.words[7] = "10.2";
	s.words[8] = "77.52";
	mu_assert(NMEA_parse_RMC(s, &out) == 0, "should be OK");

	mu_assert(out.Lat == lat, "shoud be %f", lat);
	mu_assert(out.Long == lon, "shoud be %f", lon);
	mu_assert(out.UTCTimeMs == time, "shoud be %f", time);
	mu_assert(out.Speed == speed, "%f != %f", out.Speed, speed);
	mu_assert(out.Course == course, "%f != %f", out.Course, course);

	return NULL;
}

char *test_NMEA_parse()
{
	GNSS_Data out;

	mu_assert(NMEA_parse(NULL, &out) == 1, "should be ERROR");
	mu_assert(NMEA_parse("", &out) == 1, "should be ERROR");
	mu_assert(NMEA_parse("FAKE", &out) == 1, "should be ERROR");
	mu_assert(NMEA_parse("$GNFAKE", &out) == 1, "should be ERROR");

	char gll_msg_fail[] = "$GPGLL,TRASH!";
	mu_assert(NMEA_parse(gll_msg_fail, &out) == 1, "should be ERROR");

	char gll_msg_ok[] = "$GPGLL,4717.112671,N,00833.914843,E,034138.00,A,D*7A";
	mu_assert(NMEA_parse(gll_msg_ok, &out) == 0, "should be OK");
	mu_assert(out.System == GPS, "should be GPS")

	char gga_msg_fail[] = "$GLGGA,TRASH!";
	mu_assert(NMEA_parse(gga_msg_fail, &out) == 1, "should be ERROR");

	char gga_msg_ok[] = "$GLGGA,172814.76,4717.112671,N,00833.914843,E,2,6,1.2,18.893,M,-25.669,M,2.0 0031*4F";
	mu_assert(NMEA_parse(gga_msg_ok, &out) == 0, "should be OK");
	mu_assert(out.System == GLONASS, "should be GLONASS")

	char rmc_msg_fail[] = "$GNRMC,TRASH!";
	mu_assert(NMEA_parse(rmc_msg_fail, &out) == 1, "should be ERROR");

	char rmc_msg_ok[] = "$GNRMC,123519,A,4717.112671,N,00833.914843,E,022.4,084.4,230394,003.1,W*6A";
	mu_assert(NMEA_parse(rmc_msg_ok, &out) == 0, "should be OK");
	mu_assert(out.System == COMBINATION, "should be COMBINATION")

	return NULL;
}

char *test_NMEA_checksum()
{
	mu_assert(NMEA_checksum(ublox_disable_GSA) == 0x4E, "error checksum: disable GSA");
	mu_assert(NMEA_checksum(ublox_disable_GSV) == 0x59, "error checksum: disable GSV");
	mu_assert(NMEA_checksum(ublox_disable_VTG) == 0x5E, "error checksum: disable VTG");
	mu_assert(NMEA_checksum(ublox_disable_ZDA) == 0x44, "error checksum: disable ZDA");

	return NULL;
}

char *all_tests()
{
	mu_suite_start();

	mu_run_test(test_NMEA_split_sentence);
	mu_run_test(test_NMEA_parse_system);
	mu_run_test(test_NMEA_get_message_code);
	mu_run_test(test_NMEA_parse_coordinate);
	mu_run_test(test_NMEA_parse_time);
	mu_run_test(test_NMEA_parse_knots);
	mu_run_test(test_NMEA_parse_cource);
	mu_run_test(test_NMEA_parse_GLL);
	mu_run_test(test_NMEA_parse_GGA);
	mu_run_test(test_NMEA_parse_RMC);
	mu_run_test(test_NMEA_parse);
	mu_run_test(test_NMEA_checksum);

	return NULL;
}

RUN_TESTS(all_tests);
