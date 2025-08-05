#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>

#include "editor/editor.h"
#include "log/log.h"
#include "result/result.h"

typedef struct {
	char *buf;
	size_t size;
	size_t capacity;
} row;

void row_defer(row *r) {
	free(r->buf);
	free(r);
}

typedef struct {
	row *buf;
	size_t head;
	size_t tail;
	size_t capacity;
	size_t size;
	bool is_full;
	bool is_empty;
} circular_buf;

circular_buf *circular_buf_new(size_t capacity);
bool circular_buf_push_back(circular_buf *buf, row el);
row *circular_buf_pop_back(circular_buf *buf);
bool circular_buf_push_front(circular_buf *buf, row el);
row *circular_buf_pop_front(circular_buf *buf);
void circular_buf_defer(circular_buf *buf);

circular_buf *circular_buf_new(size_t capacity) {
	circular_buf *buf = (circular_buf *)malloc(sizeof(circular_buf));
	buf->buf = (row *)malloc(sizeof(row));
	buf->capacity = capacity;
	buf->head = 0;
	buf->tail = 0;
	buf->size = 0;
	buf->is_full = false;
	buf->is_empty = true;
	return buf;
}

bool circular_buf_push_back(circular_buf *buf, row el) {
	if (buf->size == buf->capacity) {
		return false;
	}

	*(buf->buf + buf->tail) = el;
	buf->size++;
	buf->tail++;

	return true;
}

void circular_buf_defer(circular_buf *buf) {
	for (size_t i = 0; i < buf->size; i++) {
		row_defer(buf->buf + i);
	}

	free(buf->buf);
	free(buf);
}

typedef struct {
	FILE *fp;
	row* rows;
	size_t rows_count;

	struct termios orig_termios;
} editor;

result enable_raw_mode();
void disable_raw_mode();

editor *editor_new(char *file_path);
result editor_setup(editor  *e);
void editor_defer(editor *e);
void editor_append_row(editor *e, char *content, size_t size);

int main() {
	if (init_log() == FAIL) {
		char *msg = strerror(errno);
		error("init_log: %s", msg);
		exit(1);
	}

	if (enable_raw_mode() == FAIL) {
		char *msg = strerror(errno);
		error("enable_raw_mode: %s", msg);
		exit(1);
	}

	key k;
	do {
		result res = read_key(&k);
		if (res == FAIL) {
			char *msg = strerror(errno);
			error("read_key: %s", msg);
			exit(1);
		}
	} while (k != CTRL_Q);

	return 0;
}

editor *editor_new(char *file_path) {
	editor *e = (editor *)malloc(sizeof(editor));
	e->rows = NULL;
	e->rows_count = 0;

	e->fp = fopen(file_path, "r");
	if (e->fp == NULL) {
		char *msg = strerror(errno);
		error("%s: %s", file_path, msg);
		exit(1);
	}

	char *buf = NULL;
	size_t capacity;
	ssize_t size;

	if (fseek(e->fp, 0, SEEK_SET) != 0) {
		char *msg = strerror(errno);
		error("fseek: ", msg);
		exit(1);
	}

	while ((size = getline(&buf, &capacity, e->fp)) != -1) {
		editor_append_row(e, buf, (size_t)size);
	}

	for (size_t i = 0; i < e->rows_count; i++) {
		printf("%s\n", (e->rows + i)->buf);
	}

	return e;
}

struct termios orig_termios;

result enable_raw_mode() {
	if (tcgetattr(STDIN_FILENO, &orig_termios) == FAIL) {
		return FAIL;
	}

	// abc

	atexit(disable_raw_mode);

	struct termios raw = orig_termios;

	raw.c_iflag &= ~(ICRNL | IXON | BRKINT | INPCK | ISTRIP);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
	raw.c_cc[VMIN] = 0;
  	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw)) {
		return FAIL;
	}

	return SUCCESS;
}

void disable_raw_mode() {
	if (tcsetattr(STDERR_FILENO, TCSAFLUSH, &orig_termios) == FAIL) {
		char *msg = strerror(errno);
		error("disable_raw_mode: %s", msg);
		exit(1);
	}
}

void editor_defer(editor *e) {
	fclose(e->fp);
}

void editor_append_row(editor *e, char *content, size_t size) {
	e->rows = (row *)realloc(e->rows, sizeof(row) * (e->rows_count + 1));

	size_t i = e->rows_count;
	(e->rows + i)->size = size;
	(e->rows + i)->buf = (char *)malloc(size + 1);
	memcpy((e->rows + i)->buf, content, size);
	*((e->rows + i)->buf + size) = '\0';

	e->rows_count++;
}
