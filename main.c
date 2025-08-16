#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>

#include "editor/editor.h"
#include "log/log.h"

int main(int argc, char **argv) {
	if (init_log() == -1) {
		char *msg = strerror(errno);
		error("init_log: %s", msg);
		exit(1);
	}

	if (argc < 2) {
		error("missing file path");
		fprintf(stderr, "missing file path\n");
		exit(1);
	}

	char *file_path = *(argv + 1);

	if (enable_raw_mode() == -1) {
		char *msg = strerror(errno);
		error("enable_raw_mode: %s", msg);
		exit(1);
	}

	editor *e = editor_new(file_path);

	while (true) {
		editor_refresh_screen(e);
		editor_process_keypress(e);
	}

	return 0;
}
