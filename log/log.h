#ifndef LOG_H
#define LOG_H

#include "../result/result.h"

result init_log();
void info(char *msg, ...);
void warn(char *msg, ...);
void error(char *msg, ...);

#endif
