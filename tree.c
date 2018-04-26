#include <dirent.h>
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

static size_t print_directories(DIR *dh, const char *parent_dir, size_t level)
{
	struct dirent *ent;
	size_t n_files = 0;

	while ((ent = readdir(dh)) != NULL) {
		if (index(ent->d_name, '.') == ent->d_name) {
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
			n_files += 1;
		}
	}

	return n_files;
}

static void print_files(DIR *dh, size_t level, size_t n_files)
{
	struct dirent *ent;

	while ((ent = readdir(dh)) != NULL) {
		if (index(ent->d_name, '.') == ent->d_name) {
			continue;
		}

		switch (ent->d_type) {
		case DT_REG:
		case DT_LNK:
			n_files -= 1;
			const char *pref = n_files == 0
				? "  \u2514\u2500\u2500" 
				: "  \u251c\u2500\u2500";
			indent(level + 1, pref, ent->d_name);
			break;
		}
	}
}

static void print_tree(const char *dir, size_t level)
{
	DIR *dh = opendir(dir);

	if (!dh) {
		fprintf(stderr, "unable to open %s\n", dir);
		perror("opendir");
		exit(1);
	}

	size_t n_files = print_directories(dh, dir, level);

	if (!directories_only) {
		seekdir(dh, 0);
		print_files(dh, level, n_files);
	}

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
