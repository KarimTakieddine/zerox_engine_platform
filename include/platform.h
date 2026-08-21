#pragma once

#include <cstddef>

namespace platform
{
    size_t getFileSize(const char* path);
    bool readFile(const char* path, char* buffer, size_t size);
}
