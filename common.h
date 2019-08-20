
#ifndef _COMMON_H_H_
#define _COMMON_H_H_

#include <stdint.h>
#include <string>

double now();

bool   string_endswith(std::string & txt, std::string ending);
bool   string_same_with_size(const char * one, const char * other, size_t length);
bool   string_same_ignore_case(const char * one, const char * other);
bool   string_same_ignore_case_with_size(const char * one, const char * other, size_t length);

#endif // _COMMON_H_H_
