
#include "HTTPParser.h"
#include <sstream>


// Callbacks must return 0 on success. Returning a non-zero value indicates error to the parser, making it exit immediately.
int HTTPParser::request_on_message_begin(http_parser * parser) {
	// 	HTTPParser * self = (HTTPParser *) (parser->data);
	// 	printf("message begin\n");
	return 0;
}

int HTTPParser::request_on_headers_complete(http_parser * parser) {
	HTTPParser * self = (HTTPParser *) (parser->data);
	self->_headers_complete     = true;
	self->_method               = http_method_str((enum http_method) self->_parser.method);
	self->_response_status_code = self->_parser.status_code;
	std::ostringstream  version;
	version << "HTTP/" << self->_parser.http_major << "." << self->_parser.http_minor;
	self->_http_version      = version.str();
	// 	printf("headers complete\n");
	return 0;
}

int HTTPParser::request_on_message_complete(http_parser * parser) {
	// 	HTTPParser * self = (HTTPParser *) (parser->data);
	// 	printf("message complete\n");
	return 0;
}

int HTTPParser::request_on_url(http_parser * parser, const char* at, size_t length) {
	// 	printf("Url: %.*s\n", (int) length, at);
	HTTPParser * self = (HTTPParser *) (parser->data);
	self->_url += std::string(at, length);
	return 0;
}

int HTTPParser::request_on_header_field(http_parser * parser, const char* at, size_t length) {
	// 	printf("Header field: %.*s\n", (int) length, at);
	HTTPParser * self               = (HTTPParser *) (parser->data);
	if(self->_header_parsing_state == HTTPHeader::REPORTED_NONE || self->_header_parsing_state == HTTPHeader::REPORTED_VALUE) {
		self->_last_reported_field  = std::string(at, length);
	} else {
		// HTTPHeader::REPORTED_FIELD
		self->_last_reported_field += std::string(at, length);
	}
	self->_header_parsing_state     = HTTPHeader::REPORTED_FIELD;
	return 0;
}

int HTTPParser::request_on_header_value(http_parser * parser, const char* at, size_t length) {
	// 	printf("Header value: %.*s\n", (int) length, at);
	HTTPParser * self = (HTTPParser *) (parser->data);
	if(self->_header_parsing_state == HTTPHeader::REPORTED_VALUE) {
		if(self->_headers.size() > 0) {
			HTTPHeader * header = self->_headers.back();
			header->value      += std::string(at, length);
		}
	} else if(self->_header_parsing_state == HTTPHeader::REPORTED_FIELD) {
		HTTPHeader * header = new HTTPHeader();
		header->field       = self->_last_reported_field;
		header->value       = std::string(at, length);
		self->_headers.push_back(header);
	}
	self->_header_parsing_state = HTTPHeader::REPORTED_VALUE;
	return 0;
}

int HTTPParser::request_on_body(http_parser * parser, const char* at, size_t length) {
	// 	printf("Body: %.*s\n", (int) length, at);
	HTTPParser * self     = (HTTPParser *) (parser->data);
	DataBuffer & data     = self->_body;
	// // data.adjust_capacity(data.bytes_stored() + length);
	// // memcpy(data.end_of_data(), at, length);
	// // data.consume_bytes(length);
	data.write_to_end((char*) at, length);
	self->_body_complete  = (http_body_is_final(&self->_parser) == 1);
	return 0;
}

HTTPParser::HTTPParser(bool request) :
	_header_parsing_state(HTTPHeader::HeaderParsingState::REPORTED_NONE),
	_headers_complete(false),
	_body_complete(false),
	_body(4096),
	_response_status_code(0) {
	memset(&_settings, 0, sizeof(_settings));

	_settings.on_message_begin    = &(HTTPParser::request_on_message_begin);
	_settings.on_url              = &(HTTPParser::request_on_url);
	_settings.on_header_field     = &(HTTPParser::request_on_header_field);
	_settings.on_header_value     = &(HTTPParser::request_on_header_value);
	_settings.on_headers_complete = &(HTTPParser::request_on_headers_complete);
	_settings.on_body             = &(HTTPParser::request_on_body);
	_settings.on_message_complete = &(HTTPParser::request_on_message_complete);

	http_parser_init(&_parser, request ? HTTP_REQUEST : HTTP_RESPONSE);
	_parser.data = this;
}

size_t HTTPParser::process_data(const char * data, size_t data_size) {
	return http_parser_execute(&_parser, &_settings, data, data_size);
}
