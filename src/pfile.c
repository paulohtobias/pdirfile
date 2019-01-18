/**
 * Vault
 *
 * pfile.c
 *
 * Paulo Tobias
 * paulohtobias@outlook.com
 */

#include "pfile.h"

int pfile_init(pfile_t *file, const char *path) {
	memset(file, 0, sizeof *file);

	// Creating the path.
	file->path = strdup(path);

#ifndef PFILE_DONT_REPLACE_BACKSLASHES
	// Replacing '\' for '/' since most WinAPI support '/'
	for (file->path_len = 0; file->path[file->path_len] != '\0'; file->path_len++) {
		if (file->path[file->path_len] == '\\') {
			file->path[file->path_len] = '/';
		}
	}
#else
	file->path_len = strlen(file->path);
#endif // PFILE_DONT_REPLACE_BACKSLASHES

	// Removing trailing slash.
	while (file->path_len > 1 && file->path[file->path_len - 1] == PFILE_PATH_SEPARATOR) {
		file->path[file->path_len - 1] = '\0';
		file->path_len--;
	}


	// Getting the filename.
	char *filename = NULL;
	for (filename = &file->path[file->path_len]; filename != file->path && *(filename - 1) != PFILE_PATH_SEPARATOR; filename--, file->filename_len++);
	file->filename = strdup(filename);


	// Getting modification time and type.
#if (defined(_WIN32) || defined(_WIN64))
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	wchar_t *wpath = pfile_utf8_to_utf16(file->path, file->path_len + 1);
	hFind = FindFirstFileW(wpath, &ffd);
	free(wpath);

	// Error checking.
	if (hFind == INVALID_HANDLE_VALUE) {
		printf("ERROR: FindFirstFileW\n");
		return 1;
	}

	// If file is directory.
	file->is_dir = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

	// Last modification time.
	ULARGE_INTEGER ull;
	ull.LowPart = ffd.ftLastWriteTime.dwLowDateTime;
	ull.HighPart = ffd.ftLastWriteTime.dwHighDateTime;

	file->last_mod_time = (time_t) (ull.QuadPart / 10000000 - 11644473600LL);
#else
	struct stat sb;
	if (stat(file->path, &sb) != 0) {
		free(file->path);
		free(file->filename);
		return 1;
	}

	file->is_dir = S_ISDIR(sb.st_mode);
	file->last_mod_time = sb.st_mtime;
#endif // _WIN32 || _WIN64

	return 0;
}

#if defined(_WIN32) || defined(_WIN64)
int pfile_init_windows(pfile_t *file, const char *path, const WIN32_FIND_DATAW *ffd) {
	return 1;
}
#endif // _WIN32 || _WIN64

void pfile_release(pfile_t *file) {
	free(file->path);
	free(file->filename);
}

void pfile_free_list(pfile_t *files, size_t files_len) {
	for (size_t i = 0; i < files_len; i++) {
		pfile_release(files + i);
	}
	free(files);
}

int pfile_open_path(const char *path, const char *application) {
	size_t path_len = strlen(path);
	size_t command_len = path_len + 30;
#if defined(_WIN32) || defined(_WIN64)
	wchar_t command[command_len];
	wchar_t *wpath = pfile_utf8_to_utf16(path, path_len + 1);
	_swprintf(command, L"start \"\" \"%s%s\"", wpath);
	return _wsystem(command);
#else
	char command[command_len];
	sprintf(command, "xdg-open \"%s\"", path);
	return system(command);
#endif // _WIN32 || _WIN64
}

int pfile_open(const pfile_t *file, const char *application) {
	return pfile_open_path(file->path, application);
}


const char *pfile_get_path(const pfile_t *file) {
	return file->path;
}

const char *pfile_get_filename(const pfile_t *file) {
	return file->filename;
}

time_t pfile_get_last_mod_time(const pfile_t *file) {
	return file->last_mod_time;
}

bool pfile_is_dir(const pfile_t *file) {
	return file->is_dir;
}


int pfile_cmp_path(const void *f1, const void *f2) {
#ifdef STR_UTILS_H
	return strcmpn(((const pfile_t *) f1)->path, ((const pfile_t *) f2)->path);
#else
	return strcasecmp(((const pfile_t *) f1)->path, ((const pfile_t *) f2)->path);
#endif
}

int pfile_cmp_filename(const void *f1, const void *f2) {
#ifdef STR_UTILS_H
	return strcmpn(((const pfile_t *) f1)->filename, ((const pfile_t *) f2)->filename);
#else
	return strcasecmp(((const pfile_t *) f1)->filename, ((const pfile_t *) f2)->filename);
#endif
}

int pfile_cmp_time(const void *f1, const void *f2) {
	time_t t1 = ((const pfile_t *) f1)->last_mod_time;
	time_t t2 = ((const pfile_t *) f2)->last_mod_time;

	if (t1 < t2) {
		return -1;
	} else if (t2 > t1) {
		return 1;
	}
	return 0;
}


#if !defined(STR_UTILS_H) && (defined(_WIN32) || defined(_WIN64))
char *pfile_utf16_to_utf8(const wchar_t *utf16_str, int size) {
	int utf8_str_len = WideCharToMultiByte(CP_UTF8, P_FILE_UTF_FLAGS, utf16_str, size, NULL, 0, NULL, NULL);
	char *utf8_str = malloc(utf8_str_len);
	WideCharToMultiByte(CP_UTF8, P_FILE_UTF_FLAGS, utf16_str, size, utf8_str, utf8_str_len, NULL, NULL);

	return utf8_str;
}

wchar_t *pfile_utf8_to_utf16(const char *utf8_str, int size) {
	int utf16_str_len = MultiByteToWideChar(CP_UTF8, P_FILE_UTF_FLAGS, utf8_str, -1, NULL, 0);
	wchar_t *utf16_str = malloc(utf16_str_len * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, P_FILE_UTF_FLAGS, utf8_str, -1, utf16_str, utf16_str_len);

	return utf16_str;
}
#endif // !STR_UTILS_H && (_WIN32 || _WIN64)
