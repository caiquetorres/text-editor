#ifndef EDITOR_H
#define EDITOR_H

#include "../result/result.h"

typedef enum {
	NONE,
	HOME,
	END,
	LEFT_ARROW,
	CTRL_Q,
	UNKNOWN,
} key;

result read_key(key *k);

#endif
