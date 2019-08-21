
#ifndef _HTTP_PARSER_H_H
#define _HTTP_PARSER_H_H

#include "DataBuffer.h"
#include <deque>
#include <string>
#include "http-parser/http_parser.h"


class HTTPHeader
{
public:
	enum HeaderParsingState
	{
		REPORTED_NONE = 0,
		REPORTED_FIELD,
		REPORTED_VALUE
	};

	std::string field;
	std::string value;
};

class HTTPParser
{
public:
	bool                     _headers_complete;
	bool                     _body_complete;
	std::deque<HTTPHeader *> _headers;
	DataBuffer               _body;
	std::string              _url;
	std::string              _method;
	std::string              _http_version;
	int                      _response_status_code;

public:
	HTTPParser(bool request);
	size_t process_data(const char * data, size_t data_size);

	static int request_on_message_begin(http_parser * parser);
	static int request_on_headers_complete(http_parser * parser);
	static int request_on_message_complete(http_parser * parser);
	static int request_on_url(http_parser * parser, const char* at, size_t length);
	static int request_on_header_field(http_parser * parser, const char* at, size_t length);
	static int request_on_header_value(http_parser * parser, const char* at, size_t length);
	static int request_on_body(http_parser * parser, const char* at, size_t length);

private:
	HTTPParser() : _body(0) {}
	http_parser_settings           _settings;
	http_parser                    _parser;
	HTTPHeader::HeaderParsingState _header_parsing_state;
	std::string                    _last_reported_field;
};

#endif // _HTTP_PARSER_H_H
