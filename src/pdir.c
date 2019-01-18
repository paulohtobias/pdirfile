/**
 * Vault
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
		if ((match_function != NULL) && (pattern != NULL || ((flags & PDRI_NULL_PATTERN)) == 0)) {
			return match_function(filename, pattern);
		} else {
			return 1;
		}
	}
	return 0;
}

pfile_t *dir_get_files(const char *path, size_t *files_matched, int flags, const char *pattern, int (*match_function)(const char *, const char *)) {
	*files_matched = 0;

#if defined(_WIN32) || defined (_WIN32)
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	// To use FindFirstFileW we need to use the wildcard '*' at the end of the path
	char *dir = NULL;
	size_t dir_len = str_copy(&dir, path) - 1;
	if (dir[dir_len - 1] != '/' || dir[dir_len - 1] != '\\') {
		char dir_suffix[3] = {PFILE_PATH_SEPARATOR, '*', '\0'};
		dir_len = str_append(&dir, dir_suffix) - 1;
	} else {
		char dir_suffix[2] = {'*', '\0'};
		dir_len = str_append(&dir, dir_suffix) - 1;
	}

	// Converting from utf-8 to utf-16.
	wchar_t *wdir = pfile_utf8_to_utf16(dir, dir_len + 1);
	hFind = FindFirstFileW(wdir, &ffd);

	// Error checking
	if (hFind == INVALID_HANDLE_VALUE) {
		free(dir);
		free(wdir);
		return NULL;
	}

	// Removing the wildcard '*'.
	dir[dir_len - 1] = '\0';
#else
	DIR *dip;
	struct dirent *dir;

	File *files = NULL;

	dip = opendir(path);
	if(dip == NULL){
		return NULL;
	}
#endif // _WIN32 || _WIN64

	*files_matched = dir_get_files_realloc_size;

	if ((flags & PDIR_DONT_COUNT) == 0) {
		//Counting the files
		*files_matched = 0;

#if defined(_WIN32) || defined (_WIN32)
		do {
			char *fname = pfile_utf16_to_utf8(ffd.cFileName, -1);
			if (dir_get_files_match(fname, flags, pattern, match_function)) {
				(*files_matched)++;
			}
			free(fname);
		} while (FindNextFile(hFind, &ffd) != 0);

		// Checking if no file was found.
		if (GetLastError() != ERROR_NO_MORE_FILES || *files_matched == 0) {
			free(dir);
			free(wdir);
			FindClose(hFind);
			return NULL;
		}

		FindClose(hFind);
		hFind = FindFirstFileW(wdir, &ffd);

		// Error checking
		if (hFind == INVALID_HANDLE_VALUE) {
			free(dir);
			free(wdir);
			return NULL;
		}
#else
		while ((dir = readdir(dip))) {
			if (dir_get_files_match(dir->d_name, flags, pattern, pattern_function)) {
				(*files_matched)++;
			}
		}

		// Checking if no file was found.
		if (*files_matched == 0) {
			closedir(dip);
			return NULL;
		}

		rewinddir(dip);
#endif // _WIN32 || _WIN64
	}

	printf("Files found: %"PRIu64"\n", *files_matched);

	int i = 0;
	pfile_t *files = malloc((*files_matched) * sizeof(pfile_t));

	// Listing contents of the file.
#if defined(_WIN32) || defined (_WIN32)

	do {
		char *fname = pfile_utf16_to_utf8(ffd.cFileName, -1);
		if (dir_get_files_match(fname, flags, pattern, match_function)) {
			char *fpath = dir_get_full_path(dir, dir_len, fname, 0);
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
		free(fname);
	} while (FindNextFile(hFind, &ffd) != 0);

	// Checking if no file was found.
	if (GetLastError() != ERROR_NO_MORE_FILES || *files_matched == 0) {
		free(dir);
		free(wdir);
		return NULL;
	}

	FindClose(hFind);
#else
	while ((dir = readdir(dip))) {
		if (dir_get_files_match(dir->d_name, pattern, pattern_function)) {
			char *fpath = dir_get_full_path(dir, dir_len, fname, 0);
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
#endif // _WIN32 || _WIN64

	if (i < *files_matched) {
		files = realloc(files, i * sizeof(pfile_t));
	}
	*files_matched = i;

	return files;
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

#include "pdir.h"
