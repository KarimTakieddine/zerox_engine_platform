#pragma once

#include <atomic>
#include <cstddef>
#include <memory>

namespace platform
{
    struct FileWriteEvent
    {
        int eventDescriptor         { -1 };
        int watchDescriptor         { -1 };
        int shutdownDescriptor      { -1 };
        const char* targetFilename  { nullptr };
    };

    class LibraryHandle
    {
    public:
        LibraryHandle(void* handle) : m_handle(handle) { }

        void* getHandle() const { return m_handle; }

        ~LibraryHandle();
    private:
        void* m_handle{ nullptr };
    };

    size_t getFileSize(const char* path);
    bool readFile(const char* path, char* buffer, size_t size);
    std::unique_ptr<LibraryHandle> openLibrary(const char* path);
    void* loadLibraryFunction(const LibraryHandle* handle, const char* name);
    bool initFileWriteEvent(const char* parentDirectory, const char* targetFilename, FileWriteEvent* event);
    bool watchFileWrites(const FileWriteEvent* event, std::atomic<uint64_t>* writeCounter);
    void closeFileWriteEvent(const FileWriteEvent* event);
    bool stopFileWatch(const FileWriteEvent* event);
    bool copyFile(const char* source, const char* destination);
}
