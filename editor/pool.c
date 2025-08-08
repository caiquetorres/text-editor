#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include "pool.h"

#define ESC '\x1b'
#define SEQ_BUF_SIZE 3

int pool_read_key() {
	char c;
    ssize_t read_size;

    while ((read_size = read(STDIN_FILENO, &c, 1)) != 1) {
        if (read_size == -1 && errno != EAGAIN) {
            exit(1);
        }
    }

    if (c == ESC) {
        char key_seq[SEQ_BUF_SIZE] = {0};

        if (read(STDIN_FILENO, &key_seq[0], 1) != 1 ||
            read(STDIN_FILENO, &key_seq[1], 1) != 1) {
            return ESC;
        }

        if (key_seq[0] == '[') {
            if (key_seq[1] >= '0' && key_seq[1] <= '9') {
                if (read(STDIN_FILENO, &key_seq[2], 1) != 1) {
                    return ESC;
                }
                if (key_seq[2] == '~') {
                    switch (key_seq[1]) {
                        case '1': case '7': return HOME_KEY;
                        case '4': case '8': return END_KEY;
                        case '3': return DEL_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                    }
                }
            } else {
                switch (key_seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (key_seq[0] == 'O') {
            switch (key_seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }

        return ESC;
    }

    return c;
}
