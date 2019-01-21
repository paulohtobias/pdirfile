/**
 * PDirFile
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
	struct _stat sb;
	wchar_t *wpath = pfile_utf8_to_utf16(file->path, file->path_len + 1);
	#define stat(pathname, statbuf) _wstat(w ## pathname, statbuf)
	#define return free(wpath); return
#else
	struct stat sb;
#endif // _WIN32 || _WIN64

	if (stat(path, &sb) != 0) {
		free(file->path);
		free(file->filename);
		return 1;
	}

	file->is_dir = S_ISDIR(sb.st_mode);
	file->last_mod_time = sb.st_mtime;

	return 0;

#if (defined(_WIN32) || defined(_WIN64))
	#undef return
#endif // _WIN32 || _WIN64
}

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
	size_t application_len = 30;
	size_t path_len = strlen(path);
#if defined(_WIN32) || defined(_WIN64)
	bool default_application;
	if (application == NULL) {
		application = "start \"\"";
		default_application = true;
	} else {
		application_len = strlen(application);
		default_application = false;
	}
	wchar_t *wapplication = pfile_utf8_to_utf16(application, -1);
	wchar_t *wpath = pfile_utf8_to_utf16(path, path_len + 1);

	if (default_application) {
		size_t command_len = path_len + application_len;
		wchar_t command[command_len];
		_swprintf(command, L"%s \"%s\"", wapplication, wpath);

		free(wapplication);
	free(wpath);
		return _wsystem(command) == 0;
	} else {
		// additional information
		STARTUPINFOW si;
		PROCESS_INFORMATION pi;

		// set the size of the structures
		ZeroMemory( &si, sizeof(si) );
		si.cb = sizeof(si);
		ZeroMemory( &pi, sizeof(pi) );

		// start the program up
		int retval = CreateProcessW(
			wapplication,  // the path
			wpath,         // Command line
			NULL,          // Process handle not inheritable
			NULL,          // Thread handle not inheritable
			FALSE,         // Set handle inheritance to FALSE
			0,             // No creation flags
			NULL,          // Use parent's environment block
			NULL,          // Use parent's starting directory
			&si,           // Pointer to STARTUPINFO structure
			&pi            // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
		);
		// Close process and thread handles.
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		free(wapplication);
		free(wpath);
		return retval != 0;
	}
#else
	if (application == NULL) {
		application = "xdg-open";
	} else {
		application_len = strlen(application);
	}

	size_t command_len = path_len + application_len;
	char command[command_len];
	sprintf(command, "\"%s\" \"%s\"", application, path);
	return system(command) == 0;
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
		return 1;
	} else if (t2 > t1) {
		return -1;
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
