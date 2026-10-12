#pragma once

#include <chrono>
#include <sstream>
#include <string>
#include <thread>

#include "platform.h"

namespace
{
    std::string WStringToStdString(const std::wstring& wstr)
    {
        if (wstr.empty())
        {
            return std::string();
        }

        std::size_t sizeNeeded = std::wcstombs(nullptr, wstr.c_str(), 0);

        if (sizeNeeded == static_cast<std::size_t>(-1))
        {
            return std::string();
        }

        std::string strTo(sizeNeeded, '\0');

        std::wcstombs(&strTo[0], wstr.c_str(), sizeNeeded);

        return strTo;
    }

    bool WaitForFileReady(const std::string& parentDir, const std::string& fileName, DWORD timeoutMs = 10000)
    {
        const std::string fullPath = parentDir + "\\" + fileName;

        auto startTime = std::chrono::steady_clock::now();

        const DWORD sleepInterval = 50;

        while (std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() - startTime ).count() < timeoutMs)
        {
            HANDLE fileHandle = CreateFileA(
                fullPath.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0, // Exclusive access
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

            if (fileHandle != INVALID_HANDLE_VALUE)
            {
                CloseHandle(fileHandle);
                return true;
            }

            const DWORD lastError = GetLastError();
            if (lastError != ERROR_SHARING_VIOLATION && lastError != ERROR_FILE_NOT_FOUND)
            {
                return false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(sleepInterval));
        }

        return false;
    }
}

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
            CloseHandle(fileHandle);
            return 0;
        }

        CloseHandle(fileHandle);

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
        ReadFile(fileHandle, buffer, static_cast<DWORD>(size), &numberOfBytesRead, NULL);
        CloseHandle(fileHandle);

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
        event->watchDescriptor = CreateFileA(
            parentDirectory,
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            NULL
        );

        if (event->watchDescriptor == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        event->eventDescriptor = CreateEvent(NULL, TRUE, FALSE, NULL);

        if (event->eventDescriptor == NULL)
        {
            CloseHandle(event->watchDescriptor);
            return false;
        }

        event->shutdownDescriptor = CreateEvent(NULL, TRUE, FALSE, NULL);

        if (event->shutdownDescriptor == NULL)
        {
            CloseHandle(event->watchDescriptor);
            CloseHandle(event->eventDescriptor);
            return false;
        }

        event->targetFilename   = targetFilename;
        event->parentDirectory  = parentDirectory;

        return true;
    }

    bool watchFileWrites(const FileWriteEvent* event, std::atomic<uint64_t>* writeCounter)
    {
        OVERLAPPED overlapped = { };
        overlapped.hEvent = event->eventDescriptor;

        unsigned char buffer[4096] = { 0 };

        ResetEvent(event->eventDescriptor);

        DWORD bytesReturned = 0;

        BOOL success = ReadDirectoryChangesW(
            event->watchDescriptor, buffer, 4096, FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME,
            &bytesReturned, &overlapped, NULL
        );

        if (!success && GetLastError() != ERROR_IO_PENDING)
            return false;

        EventHandle events[2] = {
            event->eventDescriptor,
            event->shutdownDescriptor
        };

        const DWORD waitStatus = WaitForMultipleObjects(2, events, FALSE, INFINITE);

        if (waitStatus == WAIT_OBJECT_0 + 1)
        {
            return ( CancelIoEx(event->watchDescriptor, &overlapped) != FALSE ) &&
                ( GetOverlappedResult(event->watchDescriptor, &overlapped, &bytesReturned, TRUE) != FALSE );
        }
        else if (waitStatus == WAIT_OBJECT_0)
        {
            if (GetOverlappedResult(event->watchDescriptor, &overlapped, &bytesReturned, FALSE) && bytesReturned > 0)
            {
                unsigned char* currentRecord = buffer;

                bool fileDetected = false;

                while (currentRecord != nullptr)
                {
                    const auto* notifyInfo = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(currentRecord);

                    std::string fileName = WStringToStdString({ notifyInfo->FileName, notifyInfo->FileNameLength / sizeof(WCHAR) });

                    if (strcmp(fileName.c_str(), event->targetFilename) == 0)
                    {
                        switch (notifyInfo->Action)
                        {
                        case FILE_ACTION_ADDED:
                        case FILE_ACTION_RENAMED_NEW_NAME:
                            fileDetected = true;
                            break;
                        default:
                            break;
                        }
                    }

                    if (notifyInfo->NextEntryOffset == 0)
                    {
                        currentRecord = nullptr;
                    }
                    else
                    {
                        currentRecord += notifyInfo->NextEntryOffset;
                    }
                }

                if (fileDetected)
                {
                    if (WaitForFileReady(event->parentDirectory, event->targetFilename))
                    {
                        writeCounter->fetch_add(1, std::memory_order_release);
                    }
                }
            }
        }

        return true;
    }

    void closeFileWriteEvent(const FileWriteEvent* event)
    {
        CloseHandle(event->shutdownDescriptor);
        CloseHandle(event->eventDescriptor);
        CloseHandle(event->watchDescriptor);
    }

    bool stopFileWatch(const FileWriteEvent* event)
    {
        return SetEvent(event->shutdownDescriptor) != NULL;
    }

    bool copyFile(const char* source, const char* destination)
    {
        return CopyFileA(source, destination, FALSE) != NULL;
    }

    std::string getFormattedDateTime()
    {
        std::stringstream stream;

        SYSTEMTIME systemTime; 

        GetLocalTime(&systemTime);

        stream
            << std::to_string(systemTime.wYear) << '-'
            << std::to_string(systemTime.wMonth) << '-'
            << std::to_string(systemTime.wDay) << 'T';

        int requiredSize = GetTimeFormatEx(
            LOCALE_NAME_USER_DEFAULT,
            0,
            &systemTime,
            nullptr,
            nullptr, 0
        );

        if (requiredSize == 0)
        {
            return "NON_TIME";
        }

        std::wstring buffer(requiredSize, L'\0');
        GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &systemTime, nullptr, buffer.data(), requiredSize);

        stream
            << ::WStringToStdString(buffer) << "."
            << std::to_string(systemTime.wMilliseconds);

        return stream.str();
    }
}