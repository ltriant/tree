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
bool show_summary = false;
int32_t num_directories = 0;
int32_t num_files = 0;

static void print_tree(const char *dir, size_t level);

static void indent_file(size_t level,
		        const char *prefix,
		        const struct dirent_file *file)
{
	for (; level > 0; level--) {
		putchar(' ');
		putchar(' ');
	}

	printf("%s %s\n", prefix, file->path);
}

static void indent_dir(size_t level,
		        const char *prefix,
		        const struct dirent_dir *dir)
{
	for (; level > 0; level--) {
		putchar(' ');
		putchar(' ');
	}

	printf("%s %s\n", prefix, dir->path);
}

static void indent_link(size_t level,
		        const char *prefix,
		        const struct dirent_link *lnk)
{
	for (; level > 0; level--) {
		putchar(' ');
		putchar(' ');
	}

	printf("%s %s -> %s\n", prefix, lnk->source, lnk->destination);
}

static void indent_item(size_t level,
		        const char *prefix,
		        const struct dirent_item *item)
{
	switch (item->type) {

	case DIRENT_FILE:
	{
		struct dirent_file *last_file = item->data.file;
		indent_file(level, prefix, last_file);
		break;
	}

	case DIRENT_LINK:
	{
		struct dirent_link *last_file = item->data.link;
		indent_link(level, prefix, last_file);
		break;
	}

	case DIRENT_DIR:
	{
		struct dirent_dir *last_dir = item->data.dir;
		indent_dir(level, prefix, last_dir);
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
		if (strchr(ent->d_name, '.') == ent->d_name)
			continue;

		switch (ent->d_type) {
		case DT_DIR:
			if (show_summary)
				num_directories += 1;

			dirent_list_push_dir(&files, ent);

			break;

		case DT_FIFO:
		case DT_CHR:
		case DT_BLK:
		case DT_REG:
		case DT_SOCK:
			if (directories_only)
				break;

			if (show_summary)
				num_files += 1;

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

			size_t i = last + 1;
			do {
				i -= 1;

				const char *prefix
					= i > 0
					? "\u251c\u2500\u2500"
					: "\u2514\u2500\u2500";

				indent_item(level + 1,
					    prefix,
					    items[i]);

				if (items[i]->type == DIRENT_DIR) {
					char *new_dir;
					int rv = asprintf(&new_dir,
							  "%s/%s",
							  parent_dir,
							  items[i]->data.dir->path);

					if (!rv) {
						perror("asprintf");
						exit(1);
					}

					print_tree(new_dir, level + 1);
					free(new_dir);
				}
			} while (i > 0);
		} else {
			size_t last = files.cur - 1;

			for (size_t i = 0; i <= last; i += 1) {
				const char *prefix
					= i < last
					? "\u251c\u2500\u2500"
					: "\u2514\u2500\u2500";

				indent_item(level + 1,
					    prefix,
					    items[i]);

				if (items[i]->type == DIRENT_DIR) {
					char *new_dir;
					int rv = asprintf(&new_dir,
							  "%s/%s",
							  parent_dir,
							  items[i]->data.dir->path);

					if (!rv) {
						perror("asprintf");
						exit(1);
					}

					print_tree(new_dir, level + 1);
					free(new_dir);
				}
			}
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

static void print_summary(void)
{
	if (directories_only)
		printf("\n%d %s\n",
			num_directories,
			(num_directories == 1 ? "directory" : "directories"));
	else
		printf("\n%d %s, %d %s\n",
			num_directories,
			(num_directories == 1 ? "directory" : "directories"),
			num_files,
			(num_files == 1 ? "file" : "files"));
}

static void usage(void)
{
	printf("usage: tree [-drh] [-L level] [directory]\n");
	printf("  -d        Show directories only\n");
	printf("  -L level  Descend only `level' directories deep\n");
	printf("  -r        Sort in reverse alphabetic order\n");
	printf("  -s        Print a summary of directories and files at the end\n");
	printf("  -h        This help message\n");
}

int main(int argc, char **argv)
{
	int ch;
	while ((ch = getopt(argc, argv, "dhL:rs")) != -1) {
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

		case 's':
			show_summary = true;
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

	if (show_summary)
		print_summary();

	exit(0);
}
