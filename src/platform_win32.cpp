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

    LibraryHandle openLibrary(const char* path)
    {
        return LoadLibraryA(path);
    }

    bool closeLibrary(LibraryHandle handle)
    {
        return FreeLibrary(handle) != NULL;
    }

    FunctionHandle loadLibraryFunction(LibraryHandle handle, const char* name)
    {
        return GetProcAddress(handle, name);
    }

    bool initFileWriteEvent(const char* parentDirectory, const char* targetFilename, FileWriteEvent* event)
    {
        return true;
    }

    bool watchFileWrites(const FileWriteEvent* event, std::atomic<uint64_t>* writeCounter)
    {
        return true;
    }

    void closeFileWriteEvent(const FileWriteEvent* event)
    {
        
    }

    bool stopFileWatch(const FileWriteEvent* event)
    {
        return true;
    }

    bool copyFile(const char* source, const char* destination)
    {
        return CopyFile(source, destination, FALSE) != NULL;
    }
}