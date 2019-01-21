#include "pdir.h"

#define readstr(name, buff, alt) \
	if (argc <= alt) { \
		setbuf(stdin, NULL); \
		printf("%s: ", name); \
		fscanf(stdin, " %[^\n]", buff); \
		setbuf(stdin, NULL); \
	} else { \
		strcpy(buff, argv[alt]); \
	}

int main_cdir(int argc, char *argv[]) {
	char dir[2048];
	readstr("dir", dir, 1);

	if (dir_create(dir, PDIR_C_REC) == 0) {
		puts("success!!\n");
	}

	return 0;
}

#if defined(_WIN32) || defined(_WIN64)
int wmain(int argc, char *argv[]) {
#else
int main(int argc, char *argv[]) {
#endif // _WIN32 || _WIN64
	char dir[2048];

	//D:\Paulo\Workspace\C\dirfile\tests

	readstr("dir", dir, 1);

	printf("UTF-8 Path: '%s'\n", dir);

	size_t files_len;
	pfile_t *files = dir_get_files(dir, &files_len, 0, NULL, NULL);
	qsort(files, files_len, sizeof *files, pfile_cmp_filename);

	size_t i;
	for (i = 0; i < files_len; i++) {
		char tstr[50];
		struct tm *tmp = localtime(&files[i].last_mod_time);
		strftime(tstr, sizeof tstr, "%Y-%m-%d %H:%M:%S", tmp);
		printf("PATH: '%s'\nNAME: '%s'\nIs Dir? %d\n", pfile_get_path(files + i), pfile_get_filename(files + i), pfile_is_dir(files + i));
		printf("%s\n\n", tstr);
		//printf("'%s'\n", pfile_get_filename(files + i));
	}

	return 0;
}

#if defined(_WIN32) || defined (_WIN64)
int main2(int argc, char const *argv[]) {
	char dir[2048];

	readstr("path", dir, 1);

	printf("UTF-8 Path: '%s'\n", dir);

	// Converting path to utf-16
	wchar_t *wdir = pfile_utf8_to_utf16(dir, -1);

	// Listing contents of the file.
	DWORD dwError = 0;
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	hFind = FindFirstFileW(wdir, &ffd);
	free(wdir);

	if (hFind == INVALID_HANDLE_VALUE) {
		printf("ERROR: FindFirstFileW\n");
		return dwError;
	}

	dir[strlen(dir) - 1] = '\0';

	pfile_t file;
	int i = 0;
	do {
		char *fname = pfile_utf16_to_utf8(ffd.cFileName, -1);
		char fpath[strlen(dir) + strlen(fname) + 10];
		sprintf(fpath, "%s%s", dir, fname);
		if (pfile_init(&file, fpath) == 0) {
			printf("PATH: '%s'\nNAME: '%s'\nIs Dir? %d\n\n", pfile_get_path(&file), pfile_get_filename(&file), pfile_is_dir(&file));

			if (i == -1) {
				printf("OPEN: %d\n", pfile_open(&file, NULL));
			}
		}
		free(fname);
		i++;
	} while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES) {
		printf("Error: ERROR_NO_MORE_FILES\n");
	}

	FindClose(hFind);
	return 0;
}
#endif
