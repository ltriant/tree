#define _GNU_SOURCE

#include <dirent.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool directories_only = false;

static void print_tree(const char *dir, size_t level);

static void indent(size_t level, const char *prefix, const char *thing)
{
	for (; level > 0; level--) {
		putchar(' ');
		putchar(' ');
	}

	printf("%s %s\n", prefix, thing);
}

static void crawl_and_print(DIR *dh, const char *parent_dir, size_t level)
{
	struct dirent *ent;
	size_t cap = 64;
	size_t n = 0;
	char **files = calloc(cap, sizeof(char *));

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

		case DT_REG:
		case DT_LNK:
			if (directories_only) {
				break;
			}

			files[n] = calloc(1024, sizeof(char));
			strncpy(files[n], ent->d_name, 1024);
			n += 1;

			if (n == cap) {
				cap += 10;
				files = realloc(files, cap * sizeof(char *));
			}

			break;
		}
	}

	if (n > 0) {
		for (size_t i = 0; i < n - 1; i += 1) {
			indent(level + 1, "\u251c\u2500\u2500", files[i]);
			free(files[i]);
		}
		indent(level + 1, "\u2514\u2500\u2500", files[n-1]);
		free(files[n-1]);
	}

	free(files);
}

static void print_tree(const char *dir, size_t level)
{
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
	printf("usage: tree [-dh] [directory]\n");
	printf("  -d  directories only\n");
	printf("  -h  this help message\n");
}

int main(int argc, char **argv)
{
	int ch;
	while ((ch = getopt(argc, argv, "dh")) != -1) {
		switch (ch) {
		case 'd':
			directories_only = true;
			break;

		case 'h':
			usage();
			exit(0);
			break;
		}
	}

	argc -= optind;
	argv += optind;

	char dir[1024];

	if (argc > 0) {
		strncpy(dir, argv[0], 1024);
	} else {
		char *rv = getcwd(dir, 1024);
		
		if (!rv) {
			perror("getcwd");
			exit(1);
		}
	}

	printf("%s\n", dir);
	print_tree(dir, 0);
	exit(0);
}
