#ifndef LIST_H
#define LIST_H

#include <dirent.h>
#include <stdbool.h>
#include <sys/stat.h>

#if defined(_DARWIN_FEATURE_64_BIT_INODE)
#define DIRENT_NAME_LENGTH 1023
#elif defined(NAME_MAX)
#define DIRENT_NAME_LENGTH NAME_MAX
#else
#define DIRENT_NAME_LENGTH 255
#endif

enum dirent_type
{
	DIRENT_FILE,
	DIRENT_LINK,
	DIRENT_DIR
};

struct dirent_file
{
	char *path;
	size_t len;
	mode_t mode;
};

struct dirent_link
{
	char *source;
	size_t source_len;

	char *destination;
	size_t destination_len;
};

struct dirent_dir
{
	char *path;
	size_t len;
};

struct dirent_item
{
	enum dirent_type type;
	union
	{
		struct dirent_file *file;
		struct dirent_link *link;
		struct dirent_dir  *dir;
	} data;
};

struct dirent_list
{
	struct dirent_item **entities;
	size_t cap;
	size_t cur;
	bool full_path;
};

// Constructor
void list_init(struct dirent_list *, bool);

// Determine if the list is empty
bool list_is_empty(struct dirent_list *);

// Push a plain file onto the end of the list
void list_push_file(struct dirent_list *,
		    const char *,
		    const struct dirent *);

// Push a symlink onto the end of the list
void list_push_link(struct dirent_list *,
		    const char *,
		    const struct dirent *);

// Push a directory onto the end of the list
void list_push_dir(struct dirent_list *,
		   const char *,
		   const struct dirent *);

// Destructor
void list_destroy(struct dirent_list *);

// Sort a list of entities in ascending, case-insensitive order
void list_sort(struct dirent_list *);

// Reverse a list of entities
void list_reverse(struct dirent_list *);

#endif
