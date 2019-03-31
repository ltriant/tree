#ifndef OUT_H
#define OUT_H

#include "list.h"

#define ANSI_COLOR_RED   "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_BLUE  "\x1b[34m"
#define ANSI_COLOR_CYAN  "\x1b[36m"

#define ANSI_COLOR_BOLD  "\x1b[1m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define ANSI_COLOR_DIRECTORY ANSI_COLOR_BOLD ANSI_COLOR_BLUE
#define ANSI_COLOR_LINK_SRC  ANSI_COLOR_BOLD ANSI_COLOR_RED
#define ANSI_COLOR_LINK_DEST ANSI_COLOR_CYAN
#define ANSI_COLOR_FILE_EXEC ANSI_COLOR_BOLD ANSI_COLOR_GREEN

void indent_item(const char *, const char *, const struct dirent_item *);

#endif
