#pragma once

#include <atomic>
#include <cstddef>
#include <memory>

#include "typedefs.h"

namespace platform
{
    struct FileWriteEvent
    {
        EventHandle eventDescriptor     { NULL };
        EventHandle watchDescriptor     { NULL };
        EventHandle shutdownDescriptor  { NULL };
        const char* targetFilename      { nullptr };
    };

    class LibraryHandle
    {
    public:
        LibraryHandle(void* handle) : m_handle(handle) { }

        LibraryHandle(const LibraryHandle& other) = delete;
        LibraryHandle& operator=(const LibraryHandle& other) = delete;

        void* getHandle() const { return m_handle; }

        ~LibraryHandle();
    private:
        void* m_handle{ nullptr };
    };

    class Image
    {
    public:
        enum class Format : unsigned
        {
            UNKNOWN = 0,
            RGB     = 1,
            RGBA    = 2
        };

        Image() = default;

        Image(const Image& other) = delete;
        Image& operator=(const Image& other) = delete;

        bool load(const char* path, bool flipVertical);
        bool isLoaded() const;

        Format getFormat() const;
        int getWidth() const;
        int getHeight() const;
        void* getData() const;

        ~Image();
    private:
        void free();

        void* m_handle  { nullptr };
        int m_width     { 0 };
        int m_height    { 0 };
        Format m_format { 0 };
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
    std::unique_ptr<Image> loadImage(const char* path, bool flipVertical);
}
