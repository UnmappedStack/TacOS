#pragma once

typedef void DIR;

DIR *opendir(char *path);
struct dirent *readdir(DIR *dirp);
