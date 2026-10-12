#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

namespace platform
{
#ifdef _WIN32
	using EventHandle = HANDLE;
#elif __linux__
	using EnventHandle = int;
#endif
}
