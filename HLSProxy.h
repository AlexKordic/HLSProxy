
#ifndef _HLS_PROXY_H_
#define _HLS_PROXY_H_

#include <stdint.h>
#include <string>
#include <deque>
#include <map>


#if defined(WIN32)
#	include "winsock2.h"
#	pragma comment(lib,"Ws2_32.lib")
#	define HAVE_STRUCT_TIMESPEC
#endif

#include "pthread.h"
#include "http-parser/http_parser.h"


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


class DataBuffer
{
public:
	DataBuffer(int initial_size);
	~DataBuffer();
	
	char * start_of_data() { return _storage + _position; }
	char * end_of_data()   { return _storage + _bytes_written; }
	int    bytes_stored()  { return _bytes_written - _position; }
	void   consume_bytes(int amount) { _position += amount; }
	void   eliminate_parsed_data() { if(_position == _bytes_written) clear(); }

	void read_next_chunk(SOCKET socket);
	void reserve_capacity_from_start(int amount_needed);
	void reserve_capacity_from_end(int amount_needed);
	void clear();
	void write_to_end(const char * data, int amount);

	char * _storage;
	int    _bytes_allocated;
	int    _bytes_written;
	int    _position;

private:
	DataBuffer();
	int _CHUNK_SIZE_;
};

class RequestlineTransformation
{
public:
	RequestlineTransformation();
	void calculate(std::string input_url);

	std::string result_url;
	bool        use_ssl;
	uint32_t    cdn_port;
	std::string cdn_domain;
	std::string cdn_port_string;
};

class AbsoluteURLTransformation
{
public:
	std::string resulting_url;
	AbsoluteURLTransformation(std::string http_url, std::string listen_host, std::string listen_port_string);
};

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

	static int request_on_message_begin   (http_parser * parser);
	static int request_on_headers_complete(http_parser * parser);
	static int request_on_message_complete(http_parser * parser);
	static int request_on_url             (http_parser * parser, const char* at, size_t length);
	static int request_on_header_field    (http_parser * parser, const char* at, size_t length);
	static int request_on_header_value    (http_parser * parser, const char* at, size_t length);
	static int request_on_body            (http_parser * parser, const char* at, size_t length);

private:
	HTTPParser() : _body(0) {}
	http_parser_settings           _settings;
	http_parser                    _parser;
	HTTPHeader::HeaderParsingState _header_parsing_state;
	std::string                    _last_reported_field;
};


class PlayerActionContext
{
public:
	PlayerActionContext() {}
	~PlayerActionContext() {}
	
	std::string active_playlist;

	// // void found_variant_stream(std::string uri, std::string spec);
	// // void requested_media_playlist(std::string uri);
};

class PlayerActionTracker
{
public:
	PlayerActionTracker();
	~PlayerActionTracker();
	uint32_t              new_player();
	void                  new_player_with_number(uint32_t identity_number);
	PlayerActionContext * player_from_id(uint32_t id);
protected:
	std::map<uint32_t, PlayerActionContext *> _id_player;
	uint32_t                                  _next_player_id;
};

// class M3U8ParsingContext
// {
// public:
// 	M3U8ParsingContext() : variant_stream_parsed(false) {}
// 	~M3U8ParsingContext() {}
// 
// 	void store_variant_tag(std::string tag) {
// 		variant_stream_parsed = true;
// 		last_variant_tag      = tag;
// 	}
// 	void clear_variant_tag()       { last_variant_tag.clear(); }
// 	bool waiting_for_variant_uri() { return last_variant_tag.empty() == false; }
// 
// 	bool        variant_stream_parsed;
// 	std::string last_variant_tag;
// };


class  HLSProxyServer;
struct http_parser;
class  CDNConnection;

class HLSClient
{
public:
	HLSClient(HLSProxyServer * server, SOCKET client_socket, sockaddr_in client_address, int address_size);
	~HLSClient();
	// when run() opearation ends cleanup() is called
	void cleanup();

private:
	friend class HLSProxyServer;
	pthread_t   _thread_handle;
	enum RESPONSE_MEDIA_CONTEXT_TYPE
	{
		MEDIA_CONTEXT_UNKNOWN = 0,
		MEDIA_CONTEXT_SEGMENT,
		MEDIA_CONTEXT_MANIFEST
	};

private:
	HLSProxyServer   * _server;
	SOCKET             _socket; // player connection
	std::string        _address;
	CDNConnection    * _cdn_connection;
	RequestlineTransformation  _cdn_url;
	std::string        _cdn_request;
	double             _start_timestamp;
	void          run_player_request_parsing();
	void          run_cdn_response_parsing();
	static void * run_player_request_parsing_proxy(void * client);
	static void * run_cdn_response_parsing_proxy(void * client);
	void          parse_received_data(DataBuffer & input, int & last_newline_position, DataBuffer & output);
	void          transform_playlist_line(const char * line_start, int size, DataBuffer & output);
	void          send_to_player_nossl(const char * data, int data_size);
	void          send_to_player_nossl_chunked(const char * data, int data_size);
	void          send_to_player_terminating_chunk_nossl();
	void          media_context_type_from_request_url();
	void          media_context_type_from_response_header(HTTPParser * response_parser);

	RESPONSE_MEDIA_CONTEXT_TYPE _media_context_type;
	const char *  media_context_type_to_str();

	void          read_cookie(HTTPParser * request_parser);
	std::string   _identity_string;
	uint32_t      _identity_number;
	bool          _cookie_exists;

	// M3U8ParsingContext  _m3u8_parsing_context;
	bool          _parsing_master_playlist;
	std::string   _past_media_playlist_url;
};

class HLSProxyServer
{
public:
	//HLSProxyServer(std::string default_cdn_host, uint32_t default_cdn_port, uint32_t listen_port);
	HLSProxyServer(std::string listen_host, uint32_t listen_port);
	~HLSProxyServer();

	// Block handling connections
	void run_forever();

	PlayerActionTracker _player_action_tracker;
	std::string         _listen_host;
	std::string         _listen_port_string;
	uint32_t            _listen_port;

protected:
// 	void client_connected(HLSClient & client); // moved to HLSClient::run

// 	std::string _default_host;
// 	uint32_t    _default_port;
	SOCKET      _listen_socket;
	int         _listen_backlog_size;
	int         _recvbuf_size;
	int         _sendbuf_size;

	bool        _wsa_cleanup_needed;
	std::deque<HLSClient*> _client_list;
};

#endif
