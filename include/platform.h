#pragma once

#include <atomic>
#include <cstddef>

namespace platform
{
    struct FileWriteEvent
    {
        int eventDescriptor         { -1 };
        int watchDescriptor         { -1 };
        int shutdownDescriptor      { -1 };
        const char* targetFilename  { nullptr };
    };

    size_t getFileSize(const char* path);
    bool readFile(const char* path, char* buffer, size_t size);
    void* openLibrary(const char* path);
    void* loadLibraryFunction(void* handle, const char* name);
    bool closeLibrary(void* handle);
    bool initFileWriteEvent(const char* parentDirectory, const char* targetFilename, FileWriteEvent* event);
    bool watchFileWrites(const FileWriteEvent* event, std::atomic<uint64_t>* writeCounter);
    void closeFileWriteEvent(const FileWriteEvent* event);
    bool stopFileWatch(const FileWriteEvent* event);
}
