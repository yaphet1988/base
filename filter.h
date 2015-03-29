#pragma once
#include <stdint.h>
#include <openssl/rc4.h>
#include <openssl/sha.h>

class FilterBase
{
protected:
	void read_by_filter(char* data, size_t len){}
	void write_by_filter(char* data, size_t len){}
};

class FilterRC4
	: public FilterBase
{
public:
	FilterRC4();
	~FilterRC4();

	virtual void read_by_filter(char* data, size_t len);
	virtual void write_by_filter(char* data, size_t len);

	void set_rc4_key(const unsigned char *data, size_t len);
	bool is_encrypto();

private:
	RC4_KEY	m_rc4key;
	bool	m_bEnc;
	unsigned char* m_pEncBuf;
	size_t	m_uSize;
};

