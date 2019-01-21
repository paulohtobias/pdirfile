/**
 * PDirFile
 *
 * pdir.c
 *
 * Paulo Tobias
 * paulohtobias@outlook.com
 */

#include "pdir.h"
#include <inttypes.h>

size_t dir_get_files_realloc_size = 10; //Arbitrary value.

char *dir_get_full_path(const char *dirname, size_t dirname_len, const char *filename, size_t filename_len) {
	if (dirname_len == 0) {
		dirname_len = strlen(dirname);
	}
	if (filename_len == 0) {
		filename_len = strlen(filename);
	}

	char *full_path = malloc(dirname_len + filename_len + 1);
	sprintf(full_path, "%s%s", dirname, filename);

	return full_path;
}

int dir_get_files_match(const char *filename, int flags, const char *pattern, int (*match_function)(const char *, const char *)) {
	// Making sure it's neither '.' or '..'.
	if (strcmp(".", filename) != 0 && strcmp("..", filename) != 0) {
		// Matching the file name with the pattern.
		if ((match_function != NULL) && (pattern != NULL || ((flags & PDIR_NULL_PATTERN)) == 0)) {
			return match_function(filename, pattern);
		} else {
			return 1;
		}
	}
	return 0;
}

#if (defined(_WIN32) || defined(_WIN64))
int dir_get_files_matchW(const wchar_t *wfilename, int flags, const char *pattern, int (*match_function)(const char *, const char *)) {
	char *filename = pfile_utf16_to_utf8(wfilename, -1);

	int retval = dir_get_files_match(filename, flags, pattern, match_function);

	free(filename);
	return retval;
}
#endif // _WIN32 || _WIN64

pfile_t *dir_get_files(const char *path, size_t *files_matched, int flags, const char *pattern, int (*match_function)(const char *, const char *)) {
	*files_matched = 0;

	char *dirname = NULL;
	size_t dirname_len = 0;
	dirname_len = str_copy(&dirname, path) - 1;
#ifndef PFILE_DONT_REPLACE_BACKSLASHES
	str_replace_char(dirname, '\\', '/');
#endif

	// Appending a traiing slash to the path.
	if (dirname[dirname_len - 1] != PFILE_PATH_SEPARATOR) {
		char dirname_suffix[2] = {PFILE_PATH_SEPARATOR, '\0'};
		dirname_len = str_append(&dirname, dirname_suffix) - 1;
	}

#if (defined(_WIN32) || defined(_WIN64))
	struct _wdirent *dir;
	_WDIR *dip;
	wchar_t *wdirname = pfile_utf8_to_utf16(dirname, dirname_len + 1);

	// Redefining functions
	#define opendir(name) _wopendir(w ## name)
	#define rewinddir(dip) _wrewinddir(dip)
	#define closedir(dip) _wclosedir(dip)
	#define readdir(dir) _wreaddir(dir)
	#define dir_get_files_match dir_get_files_matchW
	#define return free(wdirname); return
#else
	struct dirent *dir;
	DIR *dip;
#endif // _WIN32 || _WIN64

	dip = opendir(dirname);
	if (dip == NULL) {
		free(dirname);
		return NULL;
	}

	*files_matched = dir_get_files_realloc_size;

	if ((flags & PDIR_DONT_COUNT) == 0) {
		//Counting the files
		*files_matched = 0;

		while ((dir = readdir(dip))) {
			if (dir_get_files_match(dir->d_name, flags, pattern, match_function)) {
				(*files_matched)++;
			}
		}

		// Checking if no file was found.
		if (*files_matched == 0) {
			closedir(dip);
			return NULL;
		}

		rewinddir(dip);
	}

	int i = 0;
	pfile_t *files = malloc((*files_matched) * sizeof(pfile_t));

	// Listing contents of the file.
	while ((dir = readdir(dip))) {
		if (dir_get_files_match(dir->d_name, flags, pattern, match_function)) {
		#if defined(_WIN32) || defined(_WIN64)
			char *filename = pfile_utf16_to_utf8(dir->d_name, -1);
			char *fpath = dir_get_full_path(dirname, dirname_len, filename, 0);
			free(filename);
		#else
			char *fpath = dir_get_full_path(dirname, dirname_len, dir->d_name, 0);
		#endif

			if (pfile_init(files + i, fpath) == 0) {
				//In case the flag DONT_COUNT_FILES was set.
				if(i + 1 == *files_matched){
					(*files_matched) += dir_get_files_realloc_size;
					files = realloc(files, (*files_matched) * sizeof(pfile_t));
				}

				i++;
			}
			free(fpath);
		}
	}
	closedir(dip);

	if (i < *files_matched) {
		files = realloc(files, i * sizeof(pfile_t));
	}
	*files_matched = i;

	return files;

#if defined(_WIN32) || defined(_WIN64)
	#undef return
#endif
}

int __dir_create(const char *path, int flags) {
	int ret;
#if defined(_WIN32) || defined(_WIN64)
	wchar_t *wpath = pfile_utf8_to_utf16(path, -1);
	ret = CreateDirectoryW(wpath, NULL);
	free(wpath);

	if (ret != 0 || (GetLastError() == ERROR_ALREADY_EXISTS && (flags & PDIR_C_EEXIST) == 0)) {
		return 0;
	}
#else
	ret = mkdir(path, 777);

	if (ret == 0 || (errno == EEXIST && (flags & PDIR_C_EEXIST) == 0)) {
		return 0;
	}
#endif // _WIN32 || _WIN64

	return -1;
}

int dir_create(const char *path, int flags) {
	if ((flags & PDIR_C_REC) == 0) {
		return __dir_create(path, flags);
	}

	int i, len = strlen(path);

	// "Remove" traling slash.
	len -= (path[len - 1] == '/' || path[len - 1] == '\\');

	char sub_dir[len + 2];
    for (i = 0; i < len; i++) {
        sub_dir[i] = path[i];
        if (path[i] == '/' || path[i] == '\\' || path[i] == '\0') {
            sub_dir[i + 1] = '\0';
            if (__dir_create(sub_dir, flags & ~PDIR_C_EEXIST) == -1) {
                return -1;
            }
        }
    }
	return __dir_create(path, flags);
}
