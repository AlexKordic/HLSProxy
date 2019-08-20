
#include "CDNConnection.h"
#include "log.h"
#include <ws2tcpip.h>


#if defined(WIN32)
#	pragma comment(lib,"mbedTLS.lib")
#endif


CDNConnection::CDNConnection() :
	_RECVBUF_SIZE(1048576),
	_SENDBUF_SIZE(1048576),
	_CHUNK_SIZE_(131072)
{
	_socket = INVALID_SOCKET;
}

CDNConnection::~CDNConnection() {
	this->close();
	closesocket(_socket);
}

void CDNConnection::connect(std::string host, uint32_t port) {
	_hostname = host;
	// DNS resolution step
	struct addrinfo   hints;
	struct addrinfo * resolved = NULL;
	ZeroMemory( &hints, sizeof(hints) );
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	char port_string[20];

	_itoa_s(port, port_string, 20, 10);

	DWORD status      = getaddrinfo(host.c_str(), port_string, &hints, &resolved);
	if(status != 0) {
		LOGE << "CDNConnection::connect DNS error. host=" << host << " port=" << port;
		throw ErrorCritical("CDNConnection::connect DNS error");
	}

	// connecting
	struct addrinfo * candidate = NULL;
	for(candidate = resolved; candidate != NULL; candidate = candidate->ai_next) {
		if(candidate->ai_family == AF_INET && candidate->ai_socktype == SOCK_STREAM) {
			_socket = socket(AF_INET, SOCK_STREAM, candidate->ai_protocol);
			if(_socket == INVALID_SOCKET) {
				throw ErrorCritical("CDNConnection::connect() socket create error");
			}
			status  = ::connect(_socket, candidate->ai_addr, (int) candidate->ai_addrlen);
			if(status == SOCKET_ERROR) {
				closesocket(_socket);
				_socket = INVALID_SOCKET;
				continue;
			}
			break;
		}
	}
	freeaddrinfo(resolved);

	if(_socket == INVALID_SOCKET) {
		LOGE << "Unable to connect to CDN " << host << ":" << port;
		throw ErrorCritical("Unable to connect to CDN");
	}

	int sockopt_status = setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, (char *) &_RECVBUF_SIZE, sizeof(int));
	if(sockopt_status == SOCKET_ERROR) {
		throw ErrorCritical("CDNConnection setsockopt(SO_RCVBUF) error");
	}
	sockopt_status = setsockopt(_socket, SOL_SOCKET, SO_SNDBUF, (char *) &_SENDBUF_SIZE, sizeof(int));
	if(sockopt_status == SOCKET_ERROR) {
		throw ErrorCritical("CDNConnection setsockopt(SO_RCVBUF) error");
	}
}

void CDNConnection::send(char * data, size_t data_size) {
	while(data_size > 0) {
		DWORD bytes_sent = ::send(_socket, data, data_size, 0);
		if(bytes_sent == SOCKET_ERROR) {
			int code = WSAGetLastError();
			LOGE << "CDNConnection::send error=" << code;
			throw EventConnectionBroken();
		}
		data             = data + bytes_sent;
		data_size       -= bytes_sent;
	}
}

size_t CDNConnection::read_next_chunk(DataBuffer * buffer) {
	buffer->reserve_capacity_from_end(_CHUNK_SIZE_);
	int count               = ::recv(_socket, buffer->end_of_data(), _CHUNK_SIZE_, 0);
	buffer->_bytes_written += count;
	return count;
}

void CDNConnection::close() {
	shutdown(_socket, SD_BOTH);
}






CDNConnectionSSL::CDNConnectionSSL() {
	_ssl_context_valid = false;
}

CDNConnectionSSL::~CDNConnectionSSL() {
	mbedtls_x509_crt_free(&_cacert);
	mbedtls_ssl_free(&_ssl);
	mbedtls_ssl_config_free(&_conf);
	mbedtls_ctr_drbg_free(&_ctr_drbg);
	mbedtls_entropy_free(&_entropy);
}

void log_mbedtls_debug(void * context, int level, const char * file, int line, const char * txt) {
	LOG << "[TLS] " << level << " " << txt << " file=" << file << " line=" << line;
}

#define SOMETHING "Mean server machine"

void CDNConnectionSSL::connect(std::string host, uint32_t port) {
	mbedtls_ssl_init       ( &_ssl );
	mbedtls_ssl_config_init( &_conf );
	mbedtls_x509_crt_init  ( &_cacert );
	mbedtls_ctr_drbg_init  ( &_ctr_drbg );
	mbedtls_entropy_init   ( &_entropy );

	int status = mbedtls_ctr_drbg_seed(
		&_ctr_drbg,
		mbedtls_entropy_func,
		&_entropy,
		(const unsigned char *) SOMETHING,
		strlen(SOMETHING)
	);
	if(status != 0) {
		LOGE << "CDNConnectionSSL::connect mbedtls_ctr_drbg_seed error=" << status;
		throw ErrorCritical("CDNConnectionSSL::connect mbedtls_ctr_drbg_seed error");
	}

	CDNConnection::connect(host, port);

	status = mbedtls_ssl_config_defaults(
		&_conf,
		MBEDTLS_SSL_IS_CLIENT,
		MBEDTLS_SSL_TRANSPORT_STREAM,
		MBEDTLS_SSL_PRESET_DEFAULT
	);
	if(status != 0) {
		LOGE << "CDNConnectionSSL::connect mbedtls_ssl_config_defaults error=" << status;
		throw ErrorCritical("CDNConnectionSSL::connect mbedtls_ssl_config_defaults error");
	}

	// TODO: proper setup
	mbedtls_ssl_conf_authmode(&_conf, MBEDTLS_SSL_VERIFY_OPTIONAL); // MBEDTLS_SSL_VERIFY_REQUIRED
	mbedtls_ssl_conf_ca_chain(&_conf, &_cacert, NULL);
	mbedtls_ssl_conf_rng(&_conf, mbedtls_ctr_drbg_random, &_ctr_drbg);
	void * debug_log_context = NULL; // context pointer not used in our simple logging logic
	mbedtls_ssl_conf_dbg(&_conf, log_mbedtls_debug, debug_log_context);

	status = mbedtls_ssl_setup(&_ssl, &_conf);
	if(status != 0) {
		LOGE << "CDNConnectionSSL::connect mbedtls_ssl_setup error=" << status;
		throw ErrorCritical("CDNConnectionSSL::connect mbedtls_ssl_setup error");
	}

	status = mbedtls_ssl_set_hostname(&_ssl, _hostname.c_str());
	if(status != 0) {
		LOGE << "CDNConnectionSSL::connect mbedtls_ssl_set_hostname error=" << status;
		throw ErrorCritical("CDNConnectionSSL::connect mbedtls_ssl_set_hostname error");
	}

	mbedtls_ssl_set_bio(&_ssl, this, CDNConnectionSSL::_ssl_send_, CDNConnectionSSL::_ssl_recv_, NULL);

	_ssl_context_valid = true;

	// handshake
	status = mbedtls_ssl_handshake(&_ssl);
	if(status != 0) {
		_ssl_context_valid = false;
		LOGE << "CDNConnectionSSL::connect mbedtls_ssl_handshake error=" << (-status);
		throw ErrorCritical("CDNConnectionSSL::connect mbedtls_ssl_handshake error");
	}

}

int CDNConnectionSSL::_ssl_send_(void *ctx, const unsigned char *buf, size_t len) {
	CDNConnectionSSL * self = (CDNConnectionSSL *) ctx;
	if(self->_ssl_context_valid == false) {
		LOGE << "_ssl_send_ on broken context";
		return 0;
	}
	DWORD bytes_sent        = ::send(self->_socket, (const char *) buf, len, 0);
	return bytes_sent;
}

int CDNConnectionSSL::_ssl_recv_(void *ctx, unsigned char *buf, size_t len) {
	CDNConnectionSSL * self = (CDNConnectionSSL *) ctx;
	if(self->_ssl_context_valid == false) {
		LOGE << "_ssl_recv_ on broken context";
		return 0;
	}
	int count               = ::recv(self->_socket, (char *) buf, len, 0);
	return count;
}

void CDNConnectionSSL::send(char * data, size_t data_size) {
	if(_ssl_context_valid == false) {
		LOGE << "send on broken context";
		throw EventConnectionBroken();
	}
	while(data_size > 0) {
		int count = mbedtls_ssl_write(&_ssl, (const unsigned char *) data, data_size);
		if(count < 1) {
			_ssl_context_valid = false;
			LOGE << "CDNConnectionSSL::send mbedtls_ssl_write error=" << count;
			throw ErrorCritical("CDNConnectionSSL::send mbedtls_ssl_write error");
		}
		data_size -= count;
		data      += count;
	}
}

size_t CDNConnectionSSL::read_next_chunk(DataBuffer * buffer) {
	if(_ssl_context_valid == false) {
		LOGE << "read_next_chunk on broken context";
		throw EventConnectionBroken();
	}

	int count               = 0;
	buffer->reserve_capacity_from_end(_CHUNK_SIZE_);
	while(true) {
		count           = mbedtls_ssl_read(
			&_ssl,
			(unsigned char *) (buffer->end_of_data()),
			_CHUNK_SIZE_
		);
		if(count == MBEDTLS_ERR_SSL_WANT_READ || count == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
		if(count == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || count == 0) {
			// // LOG << "CDN ssl closed connection";
			_ssl_context_valid = false;
			throw EventConnectionBroken();
		}
		if(count < 0) {
			LOG << "CDN ssl error=" << count;
			_ssl_context_valid = false;
			throw ErrorCritical("CDN ssl error=");
		}
		break;
	}
	buffer->_bytes_written += count;
	return count;
}

void CDNConnectionSSL::close() {
	// // mbedtls_ssl_close_notify(&_ssl);
	CDNConnection::close();
}

