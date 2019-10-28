#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "list.h"
#include "out.h"

extern int errno;

#define ITEM_SEP    "\u2502   "
#define ITEM_BLANK  "    "
#define ITEM_MIDDLE "\u251c\u2500\u2500"
#define ITEM_LAST   "\u2514\u2500\u2500"

bool show_full_path = false;
bool directories_only = false;
bool reverse_sort = false;
size_t max_depth = 0;
bool show_summary = false;
int32_t num_directories = 0;
int32_t num_files = 0;

static void crawl_and_print(const char *dir, size_t level, char *indent)
{
	if ((max_depth > 0) && (level == max_depth))
		return;

	DIR *dh = opendir(dir);

	if (!dh) {
		if (errno == EACCES || errno == EPERM)
			return;

		fprintf(stderr, "unable to open %s\n", dir);
		perror("opendir");
		exit(1);
	}

	struct dirent *ent;

	struct dirent_list files;
	list_init(&files, show_full_path);

	while ((ent = readdir(dh)) != NULL) {
		if (strchr(ent->d_name, '.') == ent->d_name)
			continue;

		switch (ent->d_type) {
		case DT_DIR:
			if (show_summary)
				num_directories += 1;

			list_push_dir(&files, dir, ent);

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

			list_push_file(&files, dir, ent);

			break;

		case DT_LNK:
			if (directories_only)
				break;

			if (show_summary)
				num_files += 1;

			list_push_link(&files, dir, ent);

			break;
		}
	}

	closedir(dh);

	if (!list_is_empty(&files)) {
		list_sort(&files);
		struct dirent_item **items = files.entities;

		if (reverse_sort) {
			list_reverse(&files);
		}

		size_t last = files.cur - 1;

		for (size_t i = 0; i <= last; i += 1) {
			const char *item_prefix
				= i < last
				? ITEM_MIDDLE
				: ITEM_LAST;

			indent_item(indent, item_prefix, items[i]);

			if (items[i]->type == DIRENT_DIR) {
				char *new_dir;

				if (show_full_path) {
					new_dir = strndup(items[i]->data.dir->path,
							  items[i]->data.dir->len);
				} else {
					int rv = asprintf(&new_dir,
							  "%s/%s",
							  dir,
							  items[i]->data.dir->path);

					if (!rv) {
						perror("asprintf");
						exit(1);
					}
				}

				char *next_indent;
				int rv = asprintf(&next_indent,
						  "%s%s",
						  indent,
						  i < last ? ITEM_SEP : ITEM_BLANK);

				if (!rv) {
					perror("asprintf");
					exit(1);
				}

				crawl_and_print(new_dir, level + 1, next_indent);

				free(new_dir);
				free(next_indent);
			}
		}
	}

	list_destroy(&files);
}

static void print_summary(void)
{
	if (directories_only) {
		printf("\n%d %s\n",
			num_directories,
			(num_directories == 1 ? "directory" : "directories"));
	} else {
		printf("\n%d %s, %d %s\n",
			num_directories,
			(num_directories == 1 ? "directory" : "directories"),
			num_files,
			(num_files == 1 ? "file" : "files"));
	}
}

static void usage(void)
{
	printf("usage: tree [-drsh] [-L level] [directory]\n");
	printf("  -d        Show directories only\n");
	printf("  -L level  Descend only `level' directories deep\n");
	printf("  -r        Sort in reverse alphabetic order\n");
	printf("  -s        Print a summary of directories and files at the end\n");
	printf("  -f        Print the full path of each file\n");
	printf("  -h        This help message\n");
}

int main(int argc, char **argv)
{
	int ch;
	while ((ch = getopt(argc, argv, "dhL:rsf")) != -1) {
		switch (ch) {
		case 'd':
			directories_only = true;
			break;

		case 'L':
		{
			int user_depth = atoi(optarg);

			if (user_depth < 0) {
				fprintf(stderr,
					"invalid depth (%i), must be greater than zero\n",
					user_depth);
				exit(1);
			}

			max_depth = user_depth;
			break;
		}

		case 'r':
			reverse_sort = true;
			break;

		case 's':
			show_summary = true;
			break;

		case 'f':
			show_full_path = true;
			break;

		case 'h':
			usage();
			exit(0);
			break;
		}
	}

	argc -= optind;
	argv += optind;

	char dir[DIRENT_NAME_LENGTH+1] = { 0 };

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
	crawl_and_print(dir, 0, "");

	if (show_summary)
		print_summary();

	exit(0);
}
