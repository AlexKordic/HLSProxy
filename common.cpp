
#include "common.h"

#ifdef _WIN32

#include <Ws2tcpip.h>
#include <mswsock.h>

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

double now() {
	struct timeval time_destination;
	gettimeofday(&time_destination, NULL);
	return ((double) time_destination.tv_sec) + (time_destination.tv_usec / 1000000.0);
}

#endif // _WIN32

