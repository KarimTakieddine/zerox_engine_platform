#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "platform.h"

namespace 
{
    constexpr size_t FILE_BLOCK_SIZE = 1 << 10;
}

namespace platform
{
    size_t getFileSize(const char* path)
    {
        struct stat fileStat;
        
        if (stat(path, &fileStat) != 0)
        {
            return 0;
        }

        return fileStat.st_size;
    }

    bool readFile(const char* path, char* buffer, size_t size)
    {
        int fileDescriptor = open(path, O_DIRECT | O_RDONLY);

        if (fileDescriptor < 0)
            return false;

        void* fileBuffer{ nullptr };
        posix_memalign(&fileBuffer, FILE_BLOCK_SIZE, FILE_BLOCK_SIZE);
        memset(fileBuffer, 0, FILE_BLOCK_SIZE);

        ssize_t readSize = read(fileDescriptor, fileBuffer, FILE_BLOCK_SIZE);

        if (readSize < 0)
        {
            free(fileBuffer);
            close(fileDescriptor);

            return false;
        }

        std::memcpy(buffer, fileBuffer, readSize);

        free(fileBuffer);
        close(fileDescriptor);

        return true;
    }
}