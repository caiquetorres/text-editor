#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "editor.h"
#include "../result/result.h"

#define CTRL(k) (k & 0x1f)

#define HOME_KEY_1 "\x1b[H"
#define HOME_KEY_2 "\x1bOH"
#define LEFT_ARROW_KEY "\x1b[D"

bool match(const char *current, const char *expected);
void debug_seq(char *seq, ssize_t size);

result read_key(key *k) {
	*k = NONE;
	char seq[9];

	ssize_t read_size = read(STDIN_FILENO, &seq, 8);
	if (read_size == -1 && errno != EAGAIN) {
		return FAIL;
	}

	if (read_size == 0) {
		return SUCCESS;
	}

	seq[read_size] = '\0';

	debug_seq(seq, read_size);
	printf("\r\n");

	*k = UNKNOWN;
	if (match(seq, LEFT_ARROW_KEY)) {
		*k = LEFT_ARROW;
	} else if ((match(seq, HOME_KEY_1) || match(seq, HOME_KEY_2))) {
		*k = HOME;
	}

	if (seq[0] == CTRL('q')) {
		*k = CTRL_Q;
	}

	return SUCCESS;
}

void debug_seq(char *seq, ssize_t size) {
	printf("[");
	for (ssize_t i = 0; i < size; i++) {
		if (i == size - 1) {
			printf("%d", *(seq + i));
		} else {
			printf("%d ", *(seq + i));
		}
	}
	printf("]");
}

bool match(const char *current, const char *expected) {
	return strncmp(current, expected, strlen(expected)) == 0;
}
