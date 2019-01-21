/**
 * PDirFile
 *
 * pdir.h
 *
 * Paulo Tobias
 * paulohtobias@outlook.com
 */

#ifndef PDIR_H
#define PDIR_H

#include <str-utils.h>
#include "pfile.h"

// OS specific headers.
#include <dirent.h>
#if !defined(_WIN32) && !defined(_WIN64)
#endif

// Flags:
#define PDIR_DONT_COUNT 1    // Array size will grow with realloc with new files found.
#define PDIR_NULL_PATTERN 2  // Treat a NULL pattern as a match and don't call match_function.
#define PDIR_C_REC 4         // Create all non-existing directories within path.
#define PDIR_C_EEXIST 8      // Treat an existing directory as an error.

// Can be changed to control the file-array growth when PDIR_DONT_COUNT is set.
extern size_t dir_get_files_realloc_size;

/**
 * Finds all files in a given directory matching the pattern using a user
 * provided matching function (e.g. regex). If match_function is NULL,
 * then all files will be returned (. and .. will always be excluded).
 * The array size will be stored into *files_matched.
 *
 * If the flag PDIR_DONT_COUNT is set, then the array will increase with the
 * value set on the global variable dir_get_files_realloc_size. You can change
 * this value before calling the function in case the directory have many files
 * and counting them first impacts too much on the performance.
 *
 * If match_function is not NULL, then it must return a non-zero for when
 * the filename matches with pattern. filename will be passed as the first
 * parameter and pattern as the second.
 *
 * return an array with files_matched Files
 */
pfile_t *dir_get_files(const char *path, size_t *files_matched, int flags,
                       const char *pattern, int (*match_function)(const char *, const char *));

/**
 * Wrapper function to work on Windows and Linux, with the additional parameter
 * flags, that can be used to control how the function works.
 */
int dir_create(const char *path, int flags);

#endif // PDIR_H
