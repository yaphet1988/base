#include "encrypt.h"
#include <algorithm>
#include <iostream>
#include <openssl/sha.h>

namespace encdec
{
std::string base64_encode(const std::string& data)
{
	return base64_encode(data.data(), data.size());
}

std::string base64_encode(const char* data, size_t len)
{
	const char EncodeTable[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	unsigned char _tmp[3] = {0};
	std::string _encode_res;
	uint32_t _num = len / 3;
	uint32_t _mod = len % 3;

	for ( uint32_t i = 1; i <= _num; ++i )
	{
		_tmp[0] = *data++;
		_tmp[1] = *data++;
		_tmp[2] = *data++;
		_encode_res += EncodeTable[_tmp[0] >> 2];
		_encode_res += EncodeTable[((_tmp[0] << 4) | (_tmp[1] >> 4)) & 0x3F];
		_encode_res += EncodeTable[((_tmp[1] << 2) | (_tmp[2] >> 6)) & 0x3F];
		_encode_res += EncodeTable[_tmp[2] & 0x3F];
	}
	switch (_mod)
	{
	case 1:
		{
			_tmp[0] = *data++;
			_encode_res += EncodeTable[(_tmp[0] & 0xFC) >> 2];
			_encode_res += EncodeTable[((_tmp[0] & 0x03) << 4)];
			_encode_res += "==";
			break;
		}
	case 2:
		{
			_tmp[0] = *data++;
			_tmp[1] = *data++;
			_encode_res += EncodeTable[(_tmp[0] & 0xFC) >> 2];
			_encode_res += EncodeTable[((_tmp[0] & 0x03) << 4) | ((_tmp[1] & 0xF0) >> 4)];
			_encode_res += EncodeTable[((_tmp[1] & 0x0F) << 2)];
			_encode_res += "=";
			break;
		}
	}

	return _encode_res;
}

std::string base64_decode(const std::string& data)
{
	return base64_decode(data.data(), data.size());
}

std::string base64_decode(const char* data, size_t len)
{
	if (len % 4 != 0)
		return "";

	char _tmp[4];
	std::string _decode_res;
	while(len > 0) 
	{
		_tmp[0] = base64_char_idx(data[0]);  
		_tmp[1] = base64_char_idx(data[1]);  
		_tmp[2] = base64_char_idx(data[2]);  
		_tmp[3] = base64_char_idx(data[3]);  

		_decode_res += (_tmp[0] << 2) | (_tmp[1] >> 4);  
		_decode_res += (_tmp[1] << 4) | (_tmp[2] >> 2);  
		_decode_res += (_tmp[2] << 6) | (_tmp[3]);  

		data += 4;  
		len -= 4;
	}
	return _decode_res;
}

uint8_t base64_char_idx(char c)
{
	if((c >= 'A') && (c <= 'Z'))  
	{   return c - 'A';  
	}else if((c >= 'a') && (c <= 'z'))  
	{   return c - 'a' + 26;  
	}else if((c >= '0') && (c <= '9'))  
	{   return c - '0' + 52;  
	}else if(c == '+')  
	{   return 62;  
	}else if(c == '/')  
	{   return 63;  
	}else if(c == '=')  
	{   return 0;  
	}  
	return 0; 
}

std::string geohash_encode(double lat, double lon, int precision)
{
	const char EncodeTable[] = "0123456789bcdefghjkmnpqrstuvwxyz";
	std::string _encode_res;
	int is_even=1, i=0;  
	double _min_lat = -90.0, _max_lat = 90.0, _min_lon = -180.0, _max_lon = 180.0, mid;  
	char bits[] = {16,8,4,2,1};
	int bit=0, ch=0;  
	while (i < precision)
	{  
		if (is_even)
		{  
			mid = (_min_lon + _max_lon) / 2;  
			if (lon > mid)
			{  
				ch |= bits[bit];  
				_min_lon = mid;  
			}
			else
				_max_lon = mid;  
		}
		else
		{  
			mid = (_min_lat + _max_lat) / 2;  
			if (lat > mid)
			{  
				ch |= bits[bit];  
				_min_lat = mid;  
			}
			else
				_max_lat = mid;  
		}  
		is_even = !is_even;
		if (bit < 4)
			bit++;  
		else
		{
			i++;
			_encode_res += EncodeTable[ch];  
			bit = 0;  
			ch = 0;  
		}  
	}  
	return _encode_res;
}

bool geohash_decode(std::string& encstr, double& out_lat, double& out_lon)
{
	if ( encstr.empty() )
	{
		out_lat = out_lon = 0.0;
		return false;
	}

	const std::string DecodeTable = "0123456789bcdefghjkmnpqrstuvwxyz";
	//double _min_lat = -90.0, _max_lat = 90.0, _min_lon = -180.0, _max_lon = 180.0, mid;
	double _lat_array[] = {-90.0, 90.0}, _lon_array[] = {-180.0, 180.0};
	char bits[] = {16,8,4,2,1};
	int is_even = 1;

	std::transform(encstr.begin(), encstr.end(), encstr.begin(), ::tolower);
	for ( std::string::iterator io = encstr.begin(); io != encstr.end(); ++io )
	{
		int pos = DecodeTable.find_first_of(*io);
		//std::cout << "char:" << *io << " pos:" << pos << std::endl;
		//std::cout << "lat-before:[" << _lat_array[0] << "~" << _lat_array[1] << "]" << std::endl;
		//std::cout << "lon-before:[" << _lon_array[0] << "~" << _lon_array[1] << "]" << std::endl;
		for ( int i = 0; i < 5; ++i )
		{
			int mask = bits[i];
			if (is_even)
			{
				_lon_array[!(pos & mask)] = (_lon_array[0] + _lon_array[1])/2;
			}
			else
			{
				_lat_array[!(pos & mask)] = (_lat_array[0] + _lat_array[1])/2;
			}
			is_even = !is_even;
		}
		//std::cout << "lat-after:[" << _lat_array[0] << "~" << _lat_array[1] << "]" << std::endl;
		//std::cout << "lon-after:[" << _lon_array[0] << "~" << _lon_array[1] << "]" << std::endl;
	}
	out_lat = (_lat_array[0] + _lat_array[1]) / 2;
	out_lon = (_lon_array[0] + _lon_array[1]) / 2;
	return true;
}

std::string sha1(const std::string& data)
{
	return sha1(data.data(), data.size());
}

std::string sha1(const char* data, size_t len)
{
	unsigned char dest[SHA_DIGEST_LENGTH];
	::SHA1((const unsigned char*)data, len, dest);
	return std::string((const char*)dest);
}
}

