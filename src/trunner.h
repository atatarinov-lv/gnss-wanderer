#ifndef _trunner
#define _trunner

#include <pthread.h>

typedef struct TRunnerConfig {
	char *name;
	void* (*func)(void* data);
} TRunnerConfig;

typedef struct TRunner {
	TRunnerConfig cfg;
	pthread_t thread;
} TRunner;

int TRunner_init(TRunnerConfig cfg, TRunner **out);
int TRunner_destroy(TRunner *r);
int TRunner_start(TRunner *r, void *arg);
int TRunner_stop(TRunner *r);

#endif
