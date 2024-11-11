#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

void print_tree(const char *path, int depth, int is_last)
{
    DIR *dir = opendir(path); // Open directory
    if (!dir)
    {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    struct stat statbuf;

    // Iterate over directory entries
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        // Create the full path to the entry
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        // Print the indentation for tree-like structure
        for (int i = 0; i < depth - 1; ++i)
        {
            printf("    ");
        }

        // Print the branch symbol
        if (depth > 0)
        {
            if (is_last)
            {
                printf("└─── ");
            }
            else
            {
                printf("├─── ");
            }
        }

        // Check if entry is a directory or a file
        if (lstat(fullpath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode))
        {
            // Directory
            printf("%s/\n", entry->d_name);
            // Recursively print the contents of the directory
            print_tree(fullpath, depth + 1, 0); // Continue with '├───'
        }
        else
        {
            // File
            printf("%s\n", entry->d_name);
        }
    }

    closedir(dir); // Close the directory stream
}

int main()
{
    const char *path = "."; // Start with current directory
    print_tree(path, 0, 1); // Start the tree, marking the root as the last element
    return 0;
}
