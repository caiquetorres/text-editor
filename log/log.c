#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>

#include "log.h"

#define MIN_SECONDS_BETWEEN_LOGS 1

FILE *LOG;
time_t last_log_time = 0;

bool can_log() {
	time_t now = time(NULL);
  	if (now - last_log_time < MIN_SECONDS_BETWEEN_LOGS) {
   		return false;
   	}
    return true;
}

result init_log() {
	LOG = fopen("editor.log", "a");
	if (LOG == NULL) {
		perror("editor.log");
		return FAIL;
	}
	last_log_time = time(NULL);
	return SUCCESS;
}

void info(char *msg, ...) {
	time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(LOG, "[%02d:%02d:%02d %04d/%02d/%02d] INFO ",
    	t->tm_hour, t->tm_min, t->tm_sec,
    	t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);

	va_list args;
	va_start(args, msg);
	vfprintf(LOG, msg, args);
	va_end(args);

	fprintf(LOG, "\n");

	fflush(LOG);
}

void error(char *msg, ...) {
 	time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(LOG, "[%02d:%02d:%02d %04d/%02d/%02d] ERRO ",
    	t->tm_hour, t->tm_min, t->tm_sec,
    	t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);

	va_list args;
	va_start(args, msg);
	vfprintf(LOG, msg, args);
	va_end(args);

	fprintf(LOG, "\n");

	fflush(LOG);
}

void warn(char *msg, ...) {
 	time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(LOG, "[%02d:%02d:%02d %04d/%02d/%02d] WARN ",
    	t->tm_hour, t->tm_min, t->tm_sec,
    	t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);

	va_list args;
	va_start(args, msg);
	vfprintf(LOG, msg, args);
	va_end(args);

	fprintf(LOG, "\n");

	fflush(LOG);
}
