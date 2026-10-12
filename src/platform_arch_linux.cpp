#include <chrono>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <liburing.h>
#include <poll.h>
#include <stdalign.h>
#include <sys/inotify.h>
#include <sys/eventfd.h>
#include <sys/sendfile.h>
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

    LibraryHandle openLibrary(const char* path)
    {
        return dlopen(path, RTLD_NOW | RTLD_LOCAL);
    }

    bool closeLibrary(LibraryHandle handle)
    {
        return dlclose(handle) != 0;
    }

    void* loadLibraryFunction(LibraryHandle handle, const char* name)
    {
        return dlsym(handle, name);
    }

    bool initFileWriteEvent(const char* parentDirectory, const char* targetFilename, FileWriteEvent* event)
    {
        const int eventDescriptor = inotify_init1(IN_NONBLOCK);
        if (eventDescriptor < 0)
        {
            return false;
        }

        event->eventDescriptor = eventDescriptor;

        const int watchDescriptor = inotify_add_watch(eventDescriptor, parentDirectory, IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_TO);
        if (watchDescriptor < 0)
        {
            close(eventDescriptor);
            return false;
        }

        event->watchDescriptor = watchDescriptor;

        const int shutdownDescriptor = eventfd(0, EFD_NONBLOCK);
        if (shutdownDescriptor < 0)
        {
            inotify_rm_watch(eventDescriptor, watchDescriptor);
            close(eventDescriptor);
            return false;
        }

        event->shutdownDescriptor   = shutdownDescriptor;
        event->targetFilename       = std::string(targetFilename);
        event->parentDirectory      = std::string(parentDirectory);

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
            const ssize_t length = read(event->eventDescriptor, buffer, sizeof(buffer));

            if (length > 0)
            {
                ssize_t i = 0;

                while (i < length)
                {
                    auto* os_event = reinterpret_cast<struct inotify_event*>(&buffer[i]);
                    
                    if ( ( os_event->mask & ( IN_CLOSE_WRITE | IN_MOVED_TO ) ) && ( strcmp(os_event->name, event->targetFilename.c_str()) == 0 ) )
                    {
                        writeCounter->fetch_add(1, std::memory_order_release);
                    }

                    i += sizeof(struct inotify_event) + os_event->len;
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

        const uint64_t u{ 1 };
        return write(event->shutdownDescriptor, &u, sizeof(uint64_t)) > 0;
    }

    bool copyFile(const char* source, const char* destination)
    {
        const int src_fd = open(source, O_RDONLY);

        if (src_fd < 0)
        {
            return false;
        }

        const int dest_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (dest_fd < 0) {
            close(src_fd);
            return false;
        }

        const size_t destinationSize = getFileSize(source);

        const loff_t bytes_sent = sendfile(dest_fd, src_fd, nullptr, destinationSize);

        const bool succeeded = ( bytes_sent == destinationSize );
        if (succeeded)
        {
            fsync(dest_fd);
        }

        close(src_fd);
        close(dest_fd);

        return succeeded;
    }

    std::string getFormattedDateTime()
    {
        std::chrono::time_point now = std::chrono::high_resolution_clock::now();

        return std::format("{0:%F}T{0:%H}:{0:%M}:{0:%OS}:{0:%z}", now);
    }
}