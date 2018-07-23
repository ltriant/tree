#ifndef DIRENT_LIST_H
#define DIRENT_LIST_H

#include <stdbool.h>

#if defined(_DARWIN_FEATURE_64_BIT_INODE)
#define DIRENT_NAME_LENGTH 1023
#elif defined(NAME_MAX)
#define DIRENT_NAME_LENGTH NAME_MAX
#else
#error "unable to determine dirent d_name length"
#define DIRENT_NAME_LENGTH 255
#endif

enum dirent_type
{
	DIRENT_FILE,
	DIRENT_LINK
};

struct dirent_file
{
	char *path;
};

struct dirent_link
{
	char *source;
	char *destination;
};

struct dirent_item
{
	void *data;  // either a dirent_file or dirent_link
	enum dirent_type type;
};

struct dirent_list
{
	struct dirent_item **entities;
	size_t cap;
	size_t cur;
};

// Constructor
void dirent_list_init(struct dirent_list *);

// Determine if the list is empty
bool dirent_list_is_empty(struct dirent_list *);

// Push a plain file onto the end of the list
void dirent_list_push_file(struct dirent_list *, const struct dirent *);

// Push a symlink onto the end of the list
void dirent_list_push_link(struct dirent_list *, const char *, const struct dirent *);

// Destructor
void dirent_list_destroy(struct dirent_list *);

// Sort a list of entities in ascending, case-insensitive order
void dirent_list_sort(struct dirent_list *);

#endif
