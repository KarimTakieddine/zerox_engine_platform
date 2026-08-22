#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <liburing.h>
#include <poll.h>
#include <stdalign.h>
#include <sys/inotify.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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

    void* openLibrary(const char* path)
    {
        return dlopen(path, RTLD_NOW | RTLD_LOCAL);
    }

    void* loadLibraryFunction(void* handle, const char* name)
    {
        if (!handle)
        {
            return nullptr;
        }

        return dlsym(handle, name);
    }

    bool closeLibrary(void* handle)
    {
        return ( dlclose(handle) == 0 );
    }

    bool initFileWriteEvent(const char* path, FileWriteEvent* event)
    {
        int eventDescriptor = inotify_init1(IN_NONBLOCK);
        if (eventDescriptor < 0)
        {
            return false;
        }

        event->eventDescriptor = eventDescriptor;

        int watchDescriptor = inotify_add_watch(eventDescriptor, path, IN_CLOSE_WRITE);
        if (watchDescriptor < 0)
        {
            close(eventDescriptor);
            return false;
        }

        event->watchDescriptor = watchDescriptor;

        int shutdownDescriptor = eventfd(0, EFD_NONBLOCK);
        if (shutdownDescriptor < 0)
        {
            inotify_rm_watch(eventDescriptor, watchDescriptor);
            close(eventDescriptor);
            return false;
        }

        event->shutdownDescriptor = shutdownDescriptor;

        return true;
    }

    bool watchFileWrites(const FileWriteEvent* event, std::atomic<uint64_t>* writeCounter)
    {
        pollfd fds[2] = {
            { .fd = event->eventDescriptor, .events = POLL_IN },
            { .fd = event->shutdownDescriptor, .events = POLL_IN }
        };

        alignas(struct inotify_event) char buffer[4096];

        const int ret = poll(fds, 2, -1);

        if (ret < 0 && errno != EINTR)
        {
            return false;
        }

        if (fds[1].revents & POLLIN)
        {
            return true;
        }

        if (fds[0].revents & POLLIN)
        {
            ssize_t length = read(event->eventDescriptor, buffer, sizeof(buffer));

            if (length > 0)
            {
                ssize_t i = 0;

                while (i < length)
                {
                    auto* event = reinterpret_cast<struct inotify_event*>(&buffer[i]);
                    
                    if (event->mask & IN_CLOSE_WRITE)
                    {
                        writeCounter->fetch_add(1, std::memory_order_release);
                    }

                    i += sizeof(struct inotify_event) + event->len;
                }
            }
        }

        return true;
    }

    void closeFileWriteEvent(const FileWriteEvent* event)
    {
        inotify_rm_watch(event->eventDescriptor, event->watchDescriptor);
        close(event->shutdownDescriptor);
        close(event->eventDescriptor);
    }

    bool stopFileWatch(const FileWriteEvent* event)
    {
        if (event->shutdownDescriptor < 0)
        {
            return false;
        }

        uint64_t u{ 1 };
        return write(event->shutdownDescriptor, &u, sizeof(uint64_t)) > 0;
    }
}