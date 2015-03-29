#pragma once
#include <stdint.h>
#include <string>

namespace encdec
{
	std::string	base64_encode(const char* data, size_t len);
	std::string base64_encode(const std::string& data);
	std::string	base64_decode(const char* data, size_t len);
	std::string base64_decode(const std::string& data);
	uint8_t		base64_char_idx(char c);

	std::string geohash_encode(double lat, double lon, int precision=20);
	bool		geohash_decode(std::string& encstr, double& out_lat, double& out_lon);

	std::string sha1(const char* data, size_t len);
	std::string sha1(const std::string& data);
}

