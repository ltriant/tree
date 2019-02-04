#define _GNU_SOURCE

#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "dirent-list.h"

// Increases the size of the list if it's currently full
static void extend_list(struct dirent_list *ents)
{
	size_t cap = ents->cap;
	if (ents->cur == cap) {
		size_t new_cap = cap + 10;
		ents->entities = realloc(ents->entities,
			new_cap * sizeof(struct dirent_item *));
		if (!ents->entities) {
			perror("realloc");
			exit(1);
		}

		ents->cap = new_cap;
	}
}

void dirent_list_init(struct dirent_list *ents)
{
	size_t cap = 64;
	ents->cap = cap;
	ents->cur = 0;
	ents->entities = calloc(cap, sizeof(struct dirent_item *));
	if (!ents->entities) {
		perror("calloc");
		exit(1);
	}
}

bool dirent_list_is_empty(struct dirent_list *ents)
{
	return ents->cur == 0;
}

void dirent_list_push_file(struct dirent_list *ents, const struct dirent *ent)
{
	// Populate the path to the file
	struct dirent_file *file = malloc(sizeof(struct dirent_file));
	if (!file) {
		perror("malloc");
		exit(1);
	}

	file->path = strndup(ent->d_name, DIRENT_NAME_LENGTH);
	if (!file->path) {
		perror("strndup");
		exit(1);
	}

	// Populate the dirent_item
	struct dirent_item *item = malloc(sizeof(struct dirent_item));
	if (!item) {
		perror("malloc");
		exit(1);
	}

	item->type = DIRENT_FILE;
	item->data.file = file;

	size_t cur = ents->cur;
	ents->entities[cur] = item;
	ents->cur = cur + 1;

	extend_list(ents);
}

void dirent_list_push_link(struct dirent_list *ents,
			   const char *parent_dir,
			   const struct dirent *ent)
{
	struct dirent_link *lnk = malloc(sizeof(struct dirent_link));
	if (!lnk) {
		perror("malloc");
		exit(1);
	}

	// Populate the source location of the link
	lnk->source = strndup(ent->d_name, DIRENT_NAME_LENGTH);
	if (!lnk->source) {
		perror("strndup");
		exit(1);
	}

	// Populate the destination of the link via readlink()
	char *fullpath;

	int as_rv = asprintf(&fullpath, "%s/%s", parent_dir, ent->d_name);
	if (!as_rv) {
		perror("asprintf");
		exit(1);
	}

	char *destination = calloc(DIRENT_NAME_LENGTH+1, sizeof(char));
	if (!destination) {
		perror("calloc");
		exit(1);
	}

	ssize_t rdlnk_rv = readlink(fullpath, destination, DIRENT_NAME_LENGTH);
	if (rdlnk_rv == -1) {
		perror("readlink");
		exit(1);
	}

	free(fullpath);
	lnk->destination = destination;

	// Populate the dirent_item
	struct dirent_item *item = malloc(sizeof(struct dirent_item));
	if (!item) {
		perror("malloc");
		exit(1);
	}

	item->type = DIRENT_LINK;
	item->data.link = lnk;

	size_t cur = ents->cur;
	ents->entities[cur] = item;
	ents->cur = cur + 1;

	extend_list(ents);
}

void dirent_list_push_dir(struct dirent_list *ents, const struct dirent *ent)
{
	// Populate the directory path
	struct dirent_dir *dir = malloc(sizeof(struct dirent_dir));
	if (!dir) {
		perror("malloc");
		exit(1);
	}

	dir->path = strndup(ent->d_name, DIRENT_NAME_LENGTH);
	if (!dir->path) {
		perror("strndup");
		exit(1);
	}

	// Populate the dirent_item
	struct dirent_item *item = malloc(sizeof(struct dirent_item));
	if (!item) {
		perror("malloc");
		exit(1);
	}

	item->type = DIRENT_DIR;
	item->data.dir = dir;

	size_t cur = ents->cur;
	ents->entities[cur] = item;
	ents->cur = cur + 1;

	extend_list(ents);
}

// Destroys a single dirent_item
static void destroy_item(struct dirent_item *item)
{
	switch (item->type) {

	case DIRENT_FILE:
	{
		struct dirent_file *file = item->data.file;

		if (file->path)
			free(file->path);

		free(item->data.file);

		break;
	}

	case DIRENT_LINK:
	{
		struct dirent_link *lnk = item->data.link;

		if (lnk->source)
			free(lnk->source);

		if (lnk->destination)
			free(lnk->destination);

		free(item->data.link);

		break;
	}

	case DIRENT_DIR:
	{
		struct dirent_dir *dir = item->data.dir;

		if (dir->path)
			free(dir->path);

		free(item->data.dir);

		break;
	}

	}
}

void dirent_list_destroy(struct dirent_list *ents)
{
	for (size_t i = 0; i < ents->cur; i += 1) {
		struct dirent_item *item = ents->entities[i];
		destroy_item(item);
		free(item);
	}

	free(ents->entities);
}

static inline char *item_string(struct dirent_item *a)
{
	char *rv;

	switch (a->type) {
	case DIRENT_FILE:
		rv = a->data.file->path;
		break;
	case DIRENT_LINK:
		rv = a->data.link->source;
		break;
	case DIRENT_DIR:
		rv = a->data.dir->path;
		break;
	default:
		rv = NULL;
	}

	return rv;
}

static inline int item_compare(struct dirent_item *a, struct dirent_item *b)
{
	char *x = item_string(a);
	char *y = item_string(b);

	// Rank directory items before non-directory items
	if (a->type == DIRENT_DIR && b->type == DIRENT_DIR)
		return strncasecmp(x, y, DIRENT_NAME_LENGTH);

	if (a->type == DIRENT_DIR)
		return -1;

	if (b->type == DIRENT_DIR)
		return 1;

	return strncasecmp(x, y, DIRENT_NAME_LENGTH);
}

void dirent_list_sort(struct dirent_list *ents)
{
	// Insertion sort, with a modification to remove a temporary variable
	// assignment in the inner loop.

	size_t i = 1;
	while (i < ents->cur) {
		struct dirent_item *item = ents->entities[i];

		ssize_t j = i - 1;
		while ((j >= 0) && (item_compare(ents->entities[j], item) > 0)) {
			ents->entities[j+1] = ents->entities[j];
			j -= 1;
		}

		ents->entities[j+1] = item;
		i += 1;
	}
}

void dirent_list_reverse(struct dirent_list *ents)
{
	for (size_t i = 0; i < (ents->cur - 1) / 2; i += 1) {
		struct dirent_item *tmp = ents->entities[i];
		ents->entities[i] = ents->entities[ents->cur - 1 - i];
		ents->entities[ents->cur - 1 - i] = tmp;
	}
}
