#include "filter.h"
#define RC4_ENC_BUF_SIZE 1024*1024

FilterRC4::FilterRC4()
: m_bEnc(false)
{
	m_pEncBuf = new unsigned char[RC4_ENC_BUF_SIZE];
	m_uSize = RC4_ENC_BUF_SIZE;
}

FilterRC4::~FilterRC4()
{
	delete[] m_pEncBuf;
}

void FilterRC4::read_by_filter(char* data, size_t len)
{
	if (m_bEnc)
		::RC4(&m_rc4key, len, (unsigned char *)data, (unsigned char *)data);
}

void FilterRC4::write_by_filter(char* data, size_t len)
{
	if (m_bEnc)
	{
		if ( m_uSize < len )
		{
			delete[] m_pEncBuf;
			m_pEncBuf = new unsigned char[len];
			m_uSize = len;
		}
		::RC4(&m_rc4key, len, (unsigned char *)data, (unsigned char *)data);
	}
}

void FilterRC4::set_rc4_key(const unsigned char *data, size_t len)
{
	::RC4_set_key(&m_rc4key, len, data);
	m_bEnc = true;
}

bool FilterRC4::is_encrypto()
{
	return m_bEnc;
}

