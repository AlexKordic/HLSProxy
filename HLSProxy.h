
#ifndef _HLS_PROXY_H_
#define _HLS_PROXY_H_

#include "url_transformation.h"
#include "DataBuffer.h"
#include <stdint.h>
#include <string>
#include <map>
#include <deque>

#if defined(WIN32)
#	include "winsock2.h"
#	pragma comment(lib,"Ws2_32.lib")
#	define HAVE_STRUCT_TIMESPEC
#endif
#include "pthread.h"



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
class  HTTPParser;

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
