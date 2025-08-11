#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "editor.h"
#include "pool.h"
#include "../log/log.h"

#define TAB 4
#define CTRL_KEY(k) (k & 0x1f)

void reset_screen();
int get_window_size(size_t *rows, size_t *cols);

typedef struct {
	char *buf;
	size_t size;
} append_buf;

append_buf *append_buf_new();
void append_buf_append(append_buf *buf, char *str, size_t size);
void append_buf_defer(append_buf *buf);

void hide_cursor(append_buf *buf);
void show_cursor(append_buf *buf);
void clean_screen(append_buf *buf);

struct row {
	char *buf;
	size_t size;
	char *rbuf;
	size_t rsize;
};

typedef enum {
	INSERT,
	NORMAL,
} mode;

typedef struct {
	size_t x, y;
} vec2;

struct editor {
	FILE *fp;
	row *rows;
	size_t rows_count;
	mode m;

	vec2 cursor;
	vec2 offset;

	size_t prev_x;

	struct screen {
		size_t width, height;
	} screen;
};

void editor_draw_rows(editor *e, append_buf *buf);
void editor_draw_status_bar(editor *e, append_buf *buf);
void editor_update_cursor(editor *e, append_buf *buf);
void editor_move_cursor(editor *e, int c);
int editor_move_cursor_up(editor *e);
int editor_move_cursor_down(editor *e);
int editor_move_cursor_left(editor *e);
int editor_move_cursor_right(editor *e);

struct termios orig_termios;

int enable_raw_mode() {
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
		return -1;
	}

	atexit(disable_raw_mode);

	struct termios raw = orig_termios;

	raw.c_iflag &= ~(ICRNL | IXON | BRKINT | INPCK | ISTRIP);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
	raw.c_cc[VMIN] = 0;
  	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw)) {
		return 0;
	}

	return 0;
}

void disable_raw_mode() {
	if (tcsetattr(STDERR_FILENO, TCSAFLUSH, &orig_termios) == -1) {
		char *msg = strerror(errno);
		error("disable_raw_mode: %s", msg);
		exit(1);
	}
}

editor *editor_new(char *file_path) {
	editor *e = (editor *)malloc(sizeof(editor));
	e->rows = NULL;
	e->rows_count = 0;

	e->cursor.x = 0;
	e->cursor.y = 0;
	e->prev_x = 0;

	e->offset.x = 0;
	e->offset.y = 0;

	e->fp = fopen(file_path, "r");
	if (e->fp == NULL) {
		char *msg = strerror(errno);
		error("%s: %s", file_path, msg);
		exit(1);
	}

	e->m = NORMAL;

	char *buf = NULL;
	size_t capacity;
	ssize_t size;

	if (fseek(e->fp, 0, SEEK_SET) != 0) {
		char *msg = strerror(errno);
		error("fseek: ", msg);
		exit(1);
	}

	while ((size = getline(&buf, &capacity, e->fp)) != -1) {
		while (size > 0 && (*(buf + size - 1) == '\n' || *(buf + size -1) == '\r')) {
			size--;
		}
		editor_append_row(e, buf, (size_t)size);
	}

	return e;
}

void editor_refresh_screen(editor *e) {
	e->screen.width = 0;
	e->screen.height = 0;

	if (get_window_size(&e->screen.height, &e->screen.width)) {
		error("get_window_size");
		exit(1);
	}

	e->screen.height--;

	append_buf *buf = append_buf_new();

	clean_screen(buf);
	hide_cursor(buf);

	editor_draw_rows(e, buf);
	editor_draw_status_bar(e, buf);
	editor_update_cursor(e, buf);

	show_cursor(buf);

	write(STDIN_FILENO, buf->buf, buf->size);

	append_buf_defer(buf);
}

int editor_move_word_back(editor *e) {
	row *r = e->rows + e->cursor.y;
	if (r->rsize == 0 || e->cursor.x == 0) {
		if (editor_move_cursor_up(e) != -1) {
			row *r = e->rows + e->cursor.y;
			e->cursor.x = r->rsize - 1;
			e->prev_x = r->rsize - 1;
			return 0;
		} else {
			return -1;
		}
	}
	for (size_t i = e->cursor.x - 1; i > 0; i--) {
		if (*(r->rbuf + i) != ' ' && *(r->rbuf + i - 1) == ' ') {
			e->cursor.x = i;
			e->prev_x = i;
			return 0;
		}
	}
	e->cursor.x = 0;
	return 0;
}

int editor_move_word(editor *e) {
	row *r = e->rows + e->cursor.y;
	for (size_t i = e->cursor.x + 1; i < r->rsize; i++) {
		if (*(r->rbuf + i) != ' ' && *(r->rbuf + i - 1) == ' ') {
			e->cursor.x = i;
			e->prev_x = i;
			return 0;
		}
	}
	if (editor_move_cursor_down(e) != -1) {
		e->cursor.x = r->rsize - 1;e->cursor.x = 0;
		e->prev_x = 0;
	}
	return 0;
}

void editor_process_keypress(editor *e) {
	int c = pool_read_key();

	if (e->m == NORMAL) {
		switch (c) {
			case 'j':
				c = ARROW_DOWN;
				break;
			case 'k':
				c = ARROW_UP;
				break;
			case 'h':
				c = ARROW_LEFT;
				break;
			case 'l':
				c = ARROW_RIGHT;
				break;
		}

		switch (c) {
			case 'q': case 'Q':
				reset_screen();
				exit(0);
				break;

			case 'i': case 'I':
				e->m = INSERT;
				break;

			case 'w':
				editor_move_word(e);
				break;

			case 'b':
				editor_move_word_back(e);
				break;

			case ARROW_UP:
			case ARROW_DOWN:
			case ARROW_RIGHT:
			case ARROW_LEFT:
				editor_move_cursor(e, c);
				break;
		}
	} else {
		switch (c) {
			case ESC:
				e->m = NORMAL;
				break;
		}
	}
}

void editor_update_cursor(editor *e, append_buf *buf) {
	char c[32];
	snprintf(c, sizeof(c), "\x1b[%zd;%zdH", (e->cursor.y - e->offset.y) + 1, (e->cursor.x - e->offset.x) + 1);
	append_buf_append(buf, c, strlen(c));
}

int editor_move_cursor_up(editor *e) {
	if (e->cursor.y == 0) {
		return -1;
	}
	if (e->cursor.y - e->offset.y == 0) {
		e->offset.y--;
	}
	e->cursor.y--;
	return 0;
}

int editor_move_cursor_down(editor *e) {
	if (e->cursor.y + 1 >= e->rows_count) {
		return -1;
	}
	if (e->cursor.y - e->offset.y >= e->screen.height - 1) {
		e->offset.y++;
	}
	e->cursor.y++;
	return 0;
}

int editor_move_cursor_left(editor *e) {
	if (e->cursor.x == 0) {
		return -1;
	}
	e->cursor.x--;
	return 0;
}

int editor_move_cursor_right(editor *e) {
	row *r = e->rows + e->cursor.y;
	if (r->rsize == 0) {
		return -1;
	}
	if (e->cursor.x == r->rsize - 1) {
		return -1;
	}
	e->cursor.x++;
	return 0;
}

void editor_move_cursor(editor *e, int c) {
	switch (c) {
		case ARROW_LEFT:
			if (editor_move_cursor_left(e) != -1) {
				e->prev_x = e->cursor.x;
			}
			break;
		case ARROW_RIGHT:
			if (editor_move_cursor_right(e) != -1) {
				e->prev_x = e->cursor.x;
			}
			break;
		case ARROW_UP:
			{
				editor_move_cursor_up(e);
				row *r = e->rows + e->cursor.y;;
				e->cursor.x = e->prev_x;
				if (r->rsize == 0) {
					e->cursor.x = 0;
				} else if (e->prev_x > r->rsize - 1) {
					e->cursor.x = r->rsize - 1;
				}
			}
			break;
		case ARROW_DOWN:
			{
				editor_move_cursor_down(e);
				row *r = e->rows + e->cursor.y;;
				e->cursor.x = e->prev_x;
				if (r->rsize == 0) {
					e->cursor.x = 0;
				} else if (e->prev_x > r->rsize - 1) {
					e->cursor.x = r->rsize - 1;
				}
				break;
			}
	}
}

void editor_draw_rows(editor *e, append_buf *buf) {
	size_t min = e->screen.height;
	if (e->rows_count < e->screen.height) {
		min = e->rows_count;
	}
	if (e->rows_count > 0) {
		for (size_t y = 0; y < min; y++) {
			row *r = (e->rows + y + e->offset.y);
			append_buf_append(buf, r->rbuf, r->rsize);
			append_buf_append(buf, "\r\n", 2);
		}
	}
	for (size_t y = min; y < e->screen.height; y++) {
		append_buf_append(buf, "~", 2);
		append_buf_append(buf, "\x1b[K", 3);
		append_buf_append(buf, "\r\n", 2);
	}
}

void editor_draw_status_bar(editor *e, append_buf *buf) {
	char mode_str[30];
	snprintf(mode_str, sizeof(mode_str), "-- %s --", e->m == NORMAL ? "NORMAL" : "INSERT");

	append_buf_append(buf, "\x1b[7m", 4);

	size_t size = strlen(mode_str);
	append_buf_append(buf, mode_str, size);

	while (size < e->screen.width) {
		append_buf_append(buf, " ", 1);
		size++;
	}

	append_buf_append(buf, "\x1b[m", 3);
}

void editor_append_row(editor *e, char *content, size_t size) {
	e->rows = (row *)realloc(e->rows, sizeof(row) * (e->rows_count + 1));
	row *r = (e->rows + e->rows_count);

	r->size = size;
	r->buf = (char *)malloc(sizeof(char) * size + 1);

	size_t tab_count = 0;
	for (size_t i = 0; i < size; i++) {
		if (*(content + i) == '\t') {
			tab_count++;
		}
	}

	r->rbuf = (char *)malloc(sizeof(char) * size + tab_count * (TAB - 1));
	r->rsize = 0;

	size_t j = 0;
	for (size_t i = 0; i < size; i++) {
		*(r->buf + i) = *(content + i);
		if (*(content + i) == '\t') {
			for (size_t k = 0; k < TAB; k++) {
				*(r->rbuf + j) = ' ';
				j++;
			}
		} else {
			*(r->rbuf + j) = *(r->buf + i);
			j++;
		}
	}

	r->rsize = j;
	e->rows_count++;
}

void editor_defer(editor *e) {
	for (size_t i = 0; i < e->rows_count; i++) {
		free((e->rows + i)->buf);
	}
	fclose(e->fp);
	free(e);
}

append_buf *append_buf_new() {
	append_buf *buf = (append_buf *)malloc(sizeof(append_buf));
	buf->buf = NULL;
	buf->size = 0;
	return buf;
}

void append_buf_append(append_buf *buf, char *str, size_t size) {
	buf->buf = (char *)realloc(buf->buf, buf->size + size);
	if (buf->buf == NULL) {
		error("append_buf_append");
		exit(1);
	}
	memcpy(buf->buf + buf->size, str, size);
	buf->size += size;
}

void append_buf_defer(append_buf *buf) {
	free(buf->buf);
}

void hide_cursor(append_buf *buf) {
	append_buf_append(buf, "\x1b[?25l", 6);
}

void show_cursor(append_buf *buf) {
	append_buf_append(buf, "\x1b[?25h", 6);
}

void clean_screen(append_buf *buf) {
	append_buf_append(buf, "\x1b[2J", 4);
	append_buf_append(buf, "\x1b[H", 3);
}

int get_window_size(size_t *rows, size_t *cols) {
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		return -1;
	}

	*cols = ws.ws_col;
	*rows = ws.ws_row;

	return 0;
}

void reset_screen() {
	write(STDOUT_FILENO, "\x1b[2J", 4); // erase display
	write(STDOUT_FILENO, "\x1b[H", 3); // reset cursor
}
