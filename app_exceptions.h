
#ifndef _APP_EXCEPTION_H_
#define _APP_EXCEPTION_H_

#include <string>

class BaseError
{
public:
	std::string reason;
	BaseError(std::string reason_) : reason(reason_) {}
};

class ErrorCritical : public BaseError
{
public:
	ErrorCritical(std::string reason_) : BaseError(reason_) {}
};

class ErrorOutOfMemory : public BaseError
{
public:
	ErrorOutOfMemory(std::string reason_) : BaseError(reason_) {}
};

class EventConnectionBroken : public BaseError
{
public:
	EventConnectionBroken() : BaseError("^ConnectionBroken^") {}
};

class EventParsingBroken : public BaseError
{
public:
	EventParsingBroken() : BaseError("^EventParsingBroken^") {}
};

#endif // _APP_EXCEPTION_H_
