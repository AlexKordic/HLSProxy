
#include "common.h"

bool string_same_ignore_case(const char * one, const char * other) {
#ifdef WIN32
	if(0 == _stricmp(one, other))
#else
	if(0 == strcasecmp(one, other))
#endif
	{
		return true;
	}
	return false;
}

bool   string_same_with_size(const char * one, const char * other, size_t length) {
	if(0 == strncmp(one, other, length))
	{
		return true;
	}
	return false;
}

bool   string_same_ignore_case_with_size(const char * one, const char * other, size_t length) {
#ifdef WIN32
	if(0 == _strnicmp(one, other, length))
#else
	if(0 == strncasecmp(one, other, length))
#endif
	{
		return true;
	}
	return false;
}

#ifdef _WIN32

#include <Ws2tcpip.h>
#include <mswsock.h>
#include <string.h> // _stricmp

double now() {
	SYSTEMTIME system_time;
	FILETIME   file_time;
	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	ULARGE_INTEGER convert;
	convert.LowPart  = file_time.dwLowDateTime;
	convert.HighPart = file_time.dwHighDateTime;
	return (convert.QuadPart / 10000) / 1000.0;
}

#else // _WIN32
#include <sys/time.h>
#include <sys/stat.h>
#include <strings.h> // strcasecmp

double now() {
	struct timeval time_destination;
	gettimeofday(&time_destination, NULL);
	return ((double) time_destination.tv_sec) + (time_destination.tv_usec / 1000000.0);
}

#endif // _WIN32

bool string_endswith(std::string & txt, std::string ending) {
	if(txt.length() >= ending.length()) {
		return (0 == txt.compare(txt.length() - ending.length(), ending.length(), ending));
	}
	return false;
}


