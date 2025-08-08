#ifndef POOL_H
#define POOL_H

#include <unistd.h>

enum key {
    ESC = 27,

	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP ,
	ARROW_DOWN,
	PAGE_UP,
	PAGE_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
};

int pool_read_key();

#endif
