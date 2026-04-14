#pragma once
#include <Windows.h>
#include <iostream>
#include <vector>

namespace io {
	void printsl( const char* input, ... )
	{
		va_list args;
		va_start( args, input );
		std::vprintf( input, args );
		va_end( args );
	}

	void print( const char* input, ... )
	{
		printsl( input );
		std::printf( "\n" );
	}

	template<typename... Args>
	void log(const std::string_view str, Args... params) {
#ifndef _REL
		static auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(handle, FOREGROUND_GREEN);
		printf("[+] ");
		SetConsoleTextAttribute(handle, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);

		std::string msg{ str };
		msg.append("\n");

		printf(msg.c_str(), std::forward<Args>(params)...);
#endif
	}

	template<typename... Args>
	void log_error(const std::string_view str, Args... params) {
#ifndef _REL
		static auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(handle, FOREGROUND_RED);
		printf("[!] ");
		SetConsoleTextAttribute(handle, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);

		std::string msg{ str };
		msg.append("\n");

		printf(msg.c_str(), std::forward<Args>(params)...);
#endif
	}

	template<typename... Args>
	void log_info(const std::string_view str, Args... params) {
#ifndef _REL
		static auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(handle, FOREGROUND_GREEN);
		printf("[+] ");
		SetConsoleTextAttribute(handle, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);

		std::string msg{ str };

		printf(msg.c_str(), std::forward<Args>(params)...);
#endif
	}

	template<typename... Args>
	void log_error_text(const std::string_view str, Args... params) {
#ifndef _REL
		static auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(handle, FOREGROUND_RED);
		printf("[!] ");
		SetConsoleTextAttribute(handle, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);

		std::string msg{ str };

		printf(msg.c_str(), std::forward<Args>(params)...);
#endif
	}

	bool read_file(const std::string_view path, std::vector<char>& out);
};  // namespace io