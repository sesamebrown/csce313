#include <fcntl.h>    // for open, O_RDONLY, O_WRONLY, O_CREAT, etc.
#include <unistd.h>   // for read, write, close
#include <sys/stat.h> // for chmod, S_IRUSR, S_IWUSR, etc.
#include <stdlib.h>   // for malloc, free

void copy(const char *src, const char *dest, size_t buffer_size)
{
    int srcfd = open(src, O_RDONLY);

    int destfd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    char *buffer = (char *)malloc(buffer_size);

    if (!buffer)
    {
        close(srcfd);
        close(destfd);
        return;
    }

    ssize_t readBytes;
    while ((readBytes = read(srcfd, buffer, buffer_size)) > 0)
    {
        ssize_t writtenBytes = write(destfd, buffer, readBytes);

        if (writtenBytes != readBytes)
        {
            break;
        }
    }

    free(buffer);
    close(srcfd);
    close(destfd);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        return 1;
    }

    const char *src = argv[1];
    const char *dest = argv[2];
    size_t buffer_size = atoi(argv[3]);

    if (buffer_size <= 0)
    {
        write(STDERR_FILENO, "Buffer size must positive\n", 27);
        return 1;
    }

    copy(src, dest, buffer_size);

    return 0;
}
