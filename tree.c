#define _GNU_SOURCE

#include <dirent.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dirent-list.h"

bool directories_only = false;
bool reverse_sort = false;
size_t max_depth = 0;

static void print_tree(const char *dir, size_t level);

static void indent(size_t level, const char *prefix, const char *thing)
{
	for (; level > 0; level--) {
		putchar(' ');
		putchar(' ');
	}

	printf("%s %s\n", prefix, thing);
}

static void indent_file(size_t level, const char *prefix, const struct dirent_file *file)
{
	for (; level > 0; level--) {
		putchar(' ');
		putchar(' ');
	}

	printf("%s %s\n", prefix, file->path);
}

static void indent_link(size_t level, const char *prefix, const struct dirent_link *lnk)
{
	for (; level > 0; level--) {
		putchar(' ');
		putchar(' ');
	}

	printf("%s %s -> %s\n", prefix, lnk->source, lnk->destination);
}

static void indent_item(size_t level, const char *prefix, const struct dirent_item *item)
{
	switch (item->type) {

	case DIRENT_FILE:
	{
		struct dirent_file *last_file = (struct dirent_file *)item->data;
		indent_file(level, prefix, last_file);
		break;
	}

	case DIRENT_LINK:
	{
		struct dirent_link *last_file = (struct dirent_link *)item->data;
		indent_link(level, prefix, last_file);
		break;
	}

	}
}

static void crawl_and_print(DIR *dh, const char *parent_dir, size_t level)
{
	struct dirent *ent;

	struct dirent_list files;
	dirent_list_init(&files);

	while ((ent = readdir(dh)) != NULL) {
		if (strchr(ent->d_name, '.') == ent->d_name) {
			continue;
		}

		switch (ent->d_type) {
		case DT_DIR:
			indent(level + 1, "\u2514\u2500\u2500", ent->d_name);

			char *new_dir;
			int rv = asprintf(&new_dir, "%s/%s", parent_dir, ent->d_name);
			if (!rv) {
				perror("asprintf");
				exit(1);
			}

			print_tree(new_dir, level + 1);
			free(new_dir);
			break;

		case DT_FIFO:
		case DT_CHR:
		case DT_BLK:
		case DT_REG:
		case DT_SOCK:
			if (directories_only)
				break;

			dirent_list_push_file(&files, ent);

			break;

		case DT_LNK:
			if (directories_only)
				break;

			dirent_list_push_link(&files, parent_dir, ent);

			break;
		}
	}

	if (!dirent_list_is_empty(&files)) {
		dirent_list_sort(&files);
		struct dirent_item **items = files.entities;

		if (reverse_sort) {
			size_t last = files.cur - 1;

			for (size_t i = last; i > 0; i -= 1) {
				indent_item(level + 1, "\u251c\u2500\u2500", items[i]);
			}

			indent_item(level + 1, "\u2514\u2500\u2500", items[0]);
		} else {
			size_t last = files.cur - 1;

			for (size_t i = 0; i < last; i += 1) {
				indent_item(level + 1, "\u251c\u2500\u2500", items[i]);
			}

			indent_item(level + 1, "\u2514\u2500\u2500", items[last]);
		}
	}

	dirent_list_destroy(&files);
}

static void print_tree(const char *dir, size_t level)
{
	if ((max_depth > 0) && (level == max_depth))
		return;

	DIR *dh = opendir(dir);

	if (!dh) {
		fprintf(stderr, "unable to open %s\n", dir);
		perror("opendir");
		exit(1);
	}

	crawl_and_print(dh, dir, level);
	closedir(dh);
}

static void usage(void)
{
	printf("usage: tree [-dh] [-L level] [directory]\n");
	printf("  -d        Show directories only\n");
	printf("  -L level  Descend only `level' directories deep\n");
	printf("  -r        Sort in reverse alphabetic order\n");
	printf("  -h        This help message\n");
}

int main(int argc, char **argv)
{
	int ch;
	while ((ch = getopt(argc, argv, "dhL:r")) != -1) {
		switch (ch) {
		case 'd':
			directories_only = true;
			break;

		case 'L':
			max_depth = atoi(optarg);
			break;

		case 'r':
			reverse_sort = true;
			break;

		case 'h':
			usage();
			exit(0);
			break;
		}
	}

	argc -= optind;
	argv += optind;

	char dir[DIRENT_NAME_LENGTH+1];
	memset(dir, 0, DIRENT_NAME_LENGTH+1);

	if (argc > 0) {
		strncpy(dir, argv[0], DIRENT_NAME_LENGTH);
	} else {
		char *rv = getcwd(dir, DIRENT_NAME_LENGTH);
		
		if (!rv) {
			perror("getcwd");
			exit(1);
		}
	}

	printf("%s\n", dir);
	print_tree(dir, 0);
	exit(0);
}
