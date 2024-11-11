#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

void print_tree(const char *path, int depth, int is_last)
{
    DIR *dir = opendir(path);

    struct dirent *entry;

    struct stat statbuf;
    struct dirent *entries[1024]; // temp array to store directories
    int count = 0;

    // count and store directory entires
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) // ignore . and ..
        {
            entries[count++] = entry;
        }
    }

    // loop through entries and prints
    for (int i = 0; i < count; i++)
    {
        entry = entries[i];
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        // print indentation
        for (int j = 0; j < depth; ++j)
        {
            printf("|   ");
        }

        // [rint branch for current entry
        if (i == count - 1)
        {
            printf("└─── ");
        }
        else
        {
            printf("├─── ");
        }

        // check if entry is a directory or a file
        if (lstat(fullpath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode))
        {
            printf("%s\n", entry->d_name);
            print_tree(fullpath, depth + 1, 0); // Not last element in directory yet
        }
        else
        {
            printf("%s\n", entry->d_name);
        }
    }
    closedir(dir);
}

int main()
{
    const char *path = ".";
    print_tree(path, 0, 1);
    return 0;
}
