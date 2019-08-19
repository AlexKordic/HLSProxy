
#ifndef _CDNCONNECTION_H_
#define _CDNCONNECTION_H_

#include "HLSProxy.h"
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"



class CDNConnection
{
public:
	CDNConnection();
	virtual ~CDNConnection();
	virtual void   connect(std::string host, uint32_t port);
	virtual void   send(char * data, size_t data_size);
	virtual size_t read_next_chunk(DataBuffer * buffer);
	virtual void   close();

protected:
	SOCKET      _socket;
	int         _CHUNK_SIZE_;
	int         _RECVBUF_SIZE;
	int         _SENDBUF_SIZE;
	std::string _hostname;
};

class CDNConnectionSSL : public CDNConnection
{
public:
	CDNConnectionSSL();
	virtual ~CDNConnectionSSL();
	virtual void   connect(std::string host, uint32_t port);
	virtual void   send(char * data, size_t data_size);
	virtual size_t read_next_chunk(DataBuffer * buffer);
	virtual void   close();

protected:
	mbedtls_entropy_context  _entropy;
	mbedtls_ctr_drbg_context _ctr_drbg;
	mbedtls_ssl_context      _ssl;
	mbedtls_ssl_config       _conf;
	mbedtls_x509_crt         _cacert;
	bool                     _ssl_context_valid;

	static int _ssl_send_(void *ctx, const unsigned char *buf, size_t len);
	static int _ssl_recv_(void *ctx, unsigned char *buf,       size_t len);
};


#endif
