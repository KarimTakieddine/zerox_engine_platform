#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

namespace platform
{
#ifdef _WIN32
	using EventHandle		= HANDLE;
	using LibraryHandle		= HMODULE;
	using FunctionHandle	= FARPROC;
#elif __linux__
	using EnventHandle		= int;
	using LibraryHandle		= void*;
	using FunctionHandle	= void*;
#endif
}
