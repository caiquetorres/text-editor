#ifndef EDITOR_H
#define EDITOR_H

#include <stddef.h>

int enable_raw_mode();
void disable_raw_mode();

typedef struct row row;
typedef struct editor editor;

editor *editor_new(char *file_path);
int editor_init(editor *e);
void editor_refresh_screen(editor *e);
void editor_process_keypress(editor *e);
void editor_append_row(editor *e, char *content, size_t size);
void editor_defer(editor *e);

#endif
