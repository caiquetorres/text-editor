#include <stdlib.h>

#include "log/log.h"

int main() {
	if (init_log() == -1) {
		exit(1);
	}

	return 0;
}
