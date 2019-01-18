/**
 * Vault
 *
 * pfile.h
 *
 * Paulo Tobias
 * paulohtobias@outlook.com
 */


#ifndef PFILE_H
#define PFILE_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <str-utils.h> // Used here for unicode conversions. This line can be safely removed.

// OS specific headers.
#if defined(_WIN32) || defined(_WIN64)
#define UNICODE
#define _UNICODE
#include <windows.h>
#else
#include <sys/stat.h>
#endif // _WIN32 || _WIN64

#ifndef PFILE_DONT_REPLACE_BACKSLASHES
#define PFILE_PATH_SEPARATOR '/'
#endif // PFILE_DONT_REPLACE_BACKSLASHES

typedef struct pfile_t {
	char *path;            // Full path to the file. Not necessarily the absolute path.
	size_t path_len;       // Size in bytes of the file path. Null-terminator not included.

	char *filename;        // Name of the file.
	size_t filename_len;   // Size in bytes of the filename. Null-terminator not included.

	time_t last_mod_time;  // Time of last modification to the file's contents.
	bool is_dir;           // true if file is a directory. false otherwise.
} pfile_t;


int pfile_init(pfile_t *file, const char *path);

#if defined(_WIN32) || defined(_WIN64)
int pfile_init_windows(pfile_t *file, const char *path, const WIN32_FIND_DATAW *ffd);
#endif // _WIN32 || _WIN64

void pfile_release(pfile_t *file);

int pfile_open_path(const char *path, const char *application);

int pfile_open(const pfile_t *file, const char *application);


const char *pfile_get_path(const pfile_t *file);

const char *pfile_get_filename(const pfile_t *file);

time_t pfile_get_last_mod_time(const pfile_t *file);

bool pfile_is_dir(const pfile_t *file);


int pfile_cmp_path(const void *f1, const void *f2);

int pfile_cmp_filename(const void *f1, const void *f2);

int pfile_cmp_time(const void *f1, const void *f2);


#if (defined(_WIN32) || defined(_WIN64))
/// UTF-8 <=> UTF-16 conversion for WinAPI compatibility.
	#ifdef STR_UTILS_H // In case str-utils.h wasn't included.
		#define P_FILE_UTF_FLAGS P_STR_UTF_FLAGS
		#define pfile_utf8_to_utf16 str_utf8_to_utf16
		#define pfile_utf16_to_utf8 str_utf16_to_utf8
	#else
		#define P_FILE_UTF_FLAGS 0
		char *pfile_utf16_to_utf8(const wchar_t *utf16_str, int size);
		wchar_t *pfile_utf8_to_utf16(const char *utf8_str, int size);
	#endif // STR_UTILS_H
#endif // _WIN32 || _WIN64

#endif // PFILE_H

/*
Some for later doc.

* if you define PFILE_DONT_REPLACE_BACKSLASHES you must also define PFILE_PATH_SEPARATOR ('/' or '\').

*/
