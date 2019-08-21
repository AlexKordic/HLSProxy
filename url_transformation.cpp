
#include "url_transformation.h"
#include "log.h"
#include "app_exceptions.h"
#include <sstream>


RequestlineTransformation::RequestlineTransformation() {
	// not used default values
	use_ssl  = false;
	cdn_port = 0;
}
void RequestlineTransformation::calculate(std::string input_url) {
	// Example:
	// "/1~443~bitdash-a.akamaihd.net/content/MI201109210084_1/m3u8s/f08e80da-bf1d-4e3d-8899-f0f6155f6efa.m3u8"
	if(input_url.size() < 7) {
		LOGE << "short input to URLTransformation: " << input_url;
		throw ErrorCritical("short input to URLTransformation");
	}
	if(input_url.at(0) != '/') {
		LOGE << "bad input to URLTransformation: " << input_url;
		throw ErrorCritical("bad input to URLTransformation");
	}
	size_t start_of_cdn_url = input_url.find('/', 1);
	if(start_of_cdn_url == std::string::npos) {
		LOGE << "bad input to URLTransformation: " << input_url;
		throw ErrorCritical("bad input to URLTransformation. second / missing");
	}
	size_t first_separator  = input_url.find('~', 1);
	if(first_separator == std::string::npos || first_separator > start_of_cdn_url) {
		LOGE << "bad input to URLTransformation: " << input_url;
		throw ErrorCritical("bad input to URLTransformation. first separator missing");
	}
	size_t second_separator = input_url.find('~', first_separator + 1);
	if(second_separator == std::string::npos || second_separator > start_of_cdn_url) {
		LOGE << "bad input to URLTransformation: " << input_url;
		throw ErrorCritical("bad input to URLTransformation. second separator missing");
	}
	std::string ssl_present = input_url.substr(1, first_separator - 1);
	cdn_port_string         = input_url.substr(first_separator + 1, second_separator - first_separator - 1);
	cdn_domain              = input_url.substr(second_separator + 1, start_of_cdn_url - second_separator - 1);
	result_url              = input_url.substr(start_of_cdn_url, input_url.size() - start_of_cdn_url); // we want to keep '/' at start_of_cdn_url
	if(ssl_present.size() != 1 || (ssl_present.at(0) != '1' && ssl_present.at(0) != '0')) {
		LOGE << "bad input to URLTransformation: " << input_url;
		throw ErrorCritical("bad input to URLTransformation. ssl_present element must be '1' or '0'");
	}
	use_ssl                 = (ssl_present.at(0) == '1');
	cdn_port                = atoi(cdn_port_string.c_str());
}

AbsoluteURLTransformation::AbsoluteURLTransformation(std::string http_url, std::string listen_host, std::string listen_port_string) {
	std::ostringstream ss;
	std::string port, domain;
	char is_ssl = '0';

	size_t end_of_protocol = http_url.find("://", 0);
	if(end_of_protocol == std::string::npos) throw EventParsingBroken();
	if(end_of_protocol == 5) is_ssl = '1';
	end_of_protocol += 3;
	size_t start_of_path   = http_url.find("/", end_of_protocol);
	if(start_of_path == std::string::npos) throw EventParsingBroken(); // TODO: this is a corner case
	size_t start_of_port   = http_url.find(":", end_of_protocol);
	if(start_of_port == std::string::npos || start_of_port > start_of_path) {
		port = (is_ssl == '0' ? "80" : "443");
		start_of_port = start_of_path; // for domain
	} else {
		port = http_url.substr(start_of_port + 1, start_of_path - start_of_port - 1);
	}
	domain = http_url.substr(end_of_protocol, start_of_port - end_of_protocol);

	// always http to player
	ss << "http://";
	// place our domain name
	ss << listen_host;
	// place our serving port
	ss << ":";
	ss << listen_port_string;
	// first element in the path is CDN destination
	ss << "/";
	// HTTPS or HTTP when proxying to CDN. player always uses HTTP to this server.
	ss << (is_ssl == '1' ? "1~" : "0~");
	// CDN port
	ss << port;
	ss << "~";
	// CDN domain
	ss << domain;
	// Add rest of original URL, firs char is /
	ss << http_url.substr(start_of_path, http_url.length() - start_of_path);

	resulting_url = ss.str();
}

