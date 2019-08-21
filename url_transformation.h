
#ifndef _URL_TRANSFORMATION_H_H
#define _URL_TRANSFORMATION_H_H

#include <string>

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

#endif // _URL_TRANSFORMATION_H_H
