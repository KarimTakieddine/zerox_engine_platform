#pragma once

#include "platform.h"

namespace platform
{
    size_t getFileSize(const char* path)
    {
        HANDLE fileHandle = CreateFileA(
            path,
            FILE_READ_ATTRIBUTES,
            NULL,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (fileHandle == INVALID_HANDLE_VALUE)
        {
            return 0;
        }

        LARGE_INTEGER fileSize = { };
        if (GetFileSizeEx(fileHandle, &fileSize) == NULL)
        {
            return 0;
        }

        return fileSize.QuadPart;
    }

    bool readFile(const char* path, char* buffer, size_t size)
    {
        HANDLE fileHandle = CreateFileA(
            path,
            GENERIC_READ,
            NULL,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        
        if (fileHandle == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        DWORD numberOfBytesRead{ 0 };
        ReadFile(fileHandle, buffer, size, &numberOfBytesRead, NULL);

        return ( size == numberOfBytesRead );
    }

    std::unique_ptr<LibraryHandle> openLibrary(const char* path)
    {

    }
    
    void* loadLibraryFunction(const LibraryHandle* handle, const char* name)
    {

    }

    bool initFileWriteEvent(const char* parentDirectory, const char* targetFilename, FileWriteEvent* event)
    {

    }

    bool watchFileWrites(const FileWriteEvent* event, std::atomic<uint64_t>* writeCounter)
    {

    }

    void closeFileWriteEvent(const FileWriteEvent* event)
    {

    }

    bool stopFileWatch(const FileWriteEvent* event)
    {

    }

    bool copyFile(const char* source, const char* destination)
    {

    }
}