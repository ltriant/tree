#include <stdio.h>
#include <unistd.h>

#include "out.h"

static void indent_item_nocolor(const char *prefix,
				const char *item_prefix,
				const struct dirent_item *item)
{
	switch (item->type) {

	case DIRENT_FILE:
	{
		struct dirent_file *file = item->data.file;

		printf("%s%s %s\n", prefix, item_prefix, file->path);
		break;
	}

	case DIRENT_LINK:
	{
		struct dirent_link *link = item->data.link;
		printf("%s%s %s -> %s\n",
		       prefix,
		       item_prefix,
		       link->source,
		       link->destination);
		break;
	}

	case DIRENT_DIR:
	{
		struct dirent_dir *dir = item->data.dir;
		printf("%s%s %s\n", prefix, item_prefix, dir->path);
		break;
	}

	}
}

static void indent_item_color(const char *prefix,
			      const char *item_prefix,
			      const struct dirent_item *item)
{
	switch (item->type) {

	case DIRENT_FILE:
	{
		struct dirent_file *file = item->data.file;

		bool is_exec = (file->mode & (S_IXGRP | S_IXUSR | S_IXOTH)) > 0;

		printf("%s%s %s%s" ANSI_COLOR_RESET "\n",
		       prefix,
		       item_prefix,
		       is_exec ? ANSI_COLOR_FILE_EXEC : ANSI_COLOR_RESET,
		       file->path);
		break;
	}

	case DIRENT_LINK:
	{
		struct dirent_link *link = item->data.link;
		printf("%s%s " ANSI_COLOR_LINK_SRC "%s " ANSI_COLOR_RESET
				ANSI_COLOR_LINK_DEST "-> %s"
				ANSI_COLOR_RESET "\n",
		       prefix,
		       item_prefix,
		       link->source,
		       link->destination);
		break;
	}

	case DIRENT_DIR:
	{
		struct dirent_dir *dir = item->data.dir;
		printf("%s%s " ANSI_COLOR_DIRECTORY "%s" ANSI_COLOR_RESET "\n",
		       prefix,
		       item_prefix,
		       dir->path);
		break;
	}

	}
}

void indent_item(const char *prefix,
		 const char *item_prefix,
		 const struct dirent_item *item)
{
	if (isatty(fileno(stdout))) {
		indent_item_color(prefix, item_prefix, item);
	}
	else {
		indent_item_nocolor(prefix, item_prefix, item);
	}
}
