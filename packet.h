#pragma once
#include "comm.h"
#include "packbuf.h"
#include <map>
#include <stdint.h>
#include <sstream>
#define HEADER_SIZE 10
#define COMMON_PACKET_SIZE 100


class Pack
{
public:
	virtual ~Pack() {}
	Pack() : _uri(0), _appid(0)			{ _buf.reserve(COMMON_PACKET_SIZE); _buf.reserve_header(HEADER_SIZE); }
	Pack(uint32_t uri, uint16_t appid=0) : _uri(uri), _appid(appid)			{ _buf.reserve(COMMON_PACKET_SIZE); _buf.reserve_header(HEADER_SIZE); }
	inline char* header()				{ return _buf.data(); }
	inline char* body()					{ return _buf.data() + HEADER_SIZE; }
	inline size_t header_size()			{ return HEADER_SIZE; }
	inline size_t body_size()			{ return _buf.size() - header_size(); }

    inline Pack& push(const void * s, size_t n)		{ _buf.append((const char *)s, n); return *this; }
    inline Pack& push(const void * s)				{ _buf.append((const char *)s); return *this; }
	inline Pack& push_uint8(uint8_t u8)				{ return push(&u8, 1); }
    inline Pack& push_uint16(uint16_t u16)			{ u16 = XHTONS(u16); return push(&u16, 2); }
    inline Pack& push_uint32(uint32_t u32)			{ u32 = XHTONL(u32); return push(&u32, 4); }
    inline Pack& push_uint64(uint64_t u64)			{ u64 = XHTONLL(u64); return push(&u64, 8); }

    inline Pack& push_str32(const std::string& str)		{ return push_str32(str.data(), str.size()); }
    inline Pack& push_str32(const void* s)			{ return push_str32(s, strlen((const char *)s)); }
    inline Pack& push_str16(const std::string& str)		{ return push_str16(str.data(), str.size()); }
    inline Pack& push_str16(const void* s)			{ return push_str16(s, strlen((const char*)s)); }

    Pack& push_str16(const void* s, size_t len)
    {
        if (len > 0xFFFF)
            throw PackError("push_str16: string too long");
        return push_uint16(uint16_t(len)).push(s, len);
    }

    Pack& push_str32(const void* s, size_t len)
    {
		if (len > 0xFFFFFFFF)
			throw PackError("push_str32: string too long");
		return push_uint32(uint32_t(len)).push(s, len);
    }

	Pack& push_header()
	{
		uint32_t _lenght = XHTONL( header_size() + body_size() );
		uint32_t _xuri = XHTONL(_uri);
		uint16_t _xappid = XHTONS(_appid);
		_buf.replace(0, (const char*)&_lenght, 4);
		_buf.replace(4, (const char*)&_xuri, 4);
		_buf.replace(8, (const char*)&_xappid, 2);
		return *this;
	}

protected:
	PackBuffer  _buf;
	uint32_t    _uri;
	uint16_t    _appid;
	
};

class Unpack
{
public:
	Unpack(const void* data, size_t size) : m_data((const char*)data), m_size(size) {}
	virtual ~Unpack() {}

	inline bool empty() const	{ return m_size == 0; }
	const char* data() const	{ return m_data; }
	inline size_t size() const	{ return m_size; }
	inline size_t pkglen() const	{ return _pkglen; }
	inline uint32_t uri() const 	{ return _uri; }
	inline uint16_t appid() const	{ return _appid; }

	uint8_t pop_uint8() const
	{
		if ( m_size < 1u )
		{
			std::ostringstream oss;
			oss << "pop_uint8: not enough data. uri:" << _uri;
			throw UnpackError(oss.str());
		}
		uint8_t i8 = *((uint8_t*)m_data);
		m_data += 1u; m_size -= 1u;
		return i8;
	}
    
	uint16_t pop_uint16() const
    {
        if ( m_size < 2u )
	{
		std::ostringstream oss;
		oss << "pop_uint16: not enough data. uri:" << _uri;
            throw UnpackError(oss.str());
	}

        uint16_t i16 = *((uint16_t*)m_data);
        i16 = XNTOHS(i16);

        m_data += 2u; m_size -= 2u;
        return i16;
    }

    uint32_t pop_uint32() const
    {
        if ( m_size < 4u )
	{
		std::ostringstream oss;
		oss << "pop_uint32: not enough data. uri:" << _uri;
            throw UnpackError(oss.str());
	}

        uint32_t i32 = *((uint32_t*)m_data);
        i32 = XNTOHL(i32);
        m_data += 4u; m_size -= 4u;
        return i32;
    }

    uint32_t peek_uint32() const {
        if (m_size < 4u)
            throw UnpackError("peek_uint32: not enough data");
        uint32_t i32 = *((uint32_t*)m_data);
        i32 = XNTOHL(i32);
        return i32;
    }
    uint64_t pop_uint64() const
    {
        if ( m_size < 8u )
	{
		std::ostringstream oss;
		oss << "pop_uint64: not enough data. uri:" << _uri;
            throw UnpackError(oss.str());
	}
        uint64_t i64 = *((uint64_t*)m_data);
        i64 = XNTOHLL(i64);
        m_data += 8u; m_size -= 8u;
        return i64;
    }

	std::string pop_str16() const
	{
		size_t _len = pop_uint16();
		if ( m_size < _len )
		{
			std::ostringstream oss;
			oss << "pop_str16: not enough data. uri:" << _uri;
			throw UnpackError(oss.str());
		}
		const char* p = m_data;
		m_data += _len;
		m_size -= _len;
		return std::string(p, _len);
	}

	void pop_str16(std::string& s) const
	{
		size_t _len = pop_uint16();
		if ( m_size < _len )
		{
			std::ostringstream oss;
			oss << "pop_str16: not enough data. uri:" << _uri;
			throw UnpackError(oss.str());
		}
		s.assign(m_data, _len);
		m_data += _len;
		m_size -= _len;
	}

	std::string pop_str32() const
	{
		size_t _len = pop_uint32();
		if ( m_size < _len )
		{
			std::ostringstream oss;
			oss << "pop_str32: not enough data. uri:" << _uri;
			throw UnpackError(oss.str());
		}
		const char* p = m_data;
		m_data += _len;
		m_size -= _len;
		return std::string(p, _len);
	}

	void pop_str32(std::string& s) const
	{
		size_t _len = pop_uint32();
		if ( m_size < _len )
		{
			std::ostringstream oss;
			oss << "pop_str32: not enough data. uri:" << _uri;
			throw UnpackError(oss.str());
		}
		s.assign(m_data, _len);
		m_data += _len;
		m_size -= _len;
	}

	void pop_header()
	{
		_pkglen = pop_uint32();
		_uri = pop_uint32();
		_appid = pop_uint16();
	}

private:
	mutable const char* m_data;
	mutable size_t m_size;
	size_t	_pkglen;
	size_t	_uri;
	uint16_t _appid;
};

struct Marshallable 
{
    virtual void marshal(Pack& pk) const = 0;
    virtual void unmarshal(const Unpack& up) = 0;
	virtual void unmarshal_incomplete(const Unpack& up){}
};

//--------------------------------------------  pack operator  ------------------------------------------------------------------
inline Pack& operator << (Pack& p, bool sign)				{ p.push_uint8(sign ? 1 : 0); return p; }
inline Pack& operator << (Pack& p, uint8_t i8)				{ p.push_uint8(i8); return p; }
inline Pack& operator << (Pack& p, uint16_t i16)			{ p.push_uint16(i16); return p; }
inline Pack& operator << (Pack& p, uint32_t i32)			{ p.push_uint32(i32); return p; }
inline Pack& operator << (Pack& p, uint64_t i64)			{ p.push_uint64(i64); return p; }
inline Pack& operator << (Pack& p, const std::string& str)		{ p.push_str16(str); return p; }
inline Pack& operator << (Pack& p, const Marshallable& m)		{ m.marshal(p); return p; }

//------------------------------------------- unpack operator --------------------------------------------------------------------
inline const Unpack& operator >> (const Unpack& up, bool& sign)		{ sign =  (up.pop_uint8() == 0) ? false : true; return up; }
inline const Unpack& operator >> (const Unpack& up, uint8_t& i8)	{ i8 =  up.pop_uint8(); return up; }
inline const Unpack& operator >> (const Unpack& up, uint16_t& i16)	{ i16 = up.pop_uint16(); return up; }
inline const Unpack& operator >> (const Unpack& up, uint32_t& i32)	{ i32 = up.pop_uint32(); return up; }
inline const Unpack& operator >> (const Unpack& up, uint64_t& i64)	{ i64 = up.pop_uint64(); return up; }
inline const Unpack& operator >> (const Unpack& up, std::string& str)	{ str = up.pop_str16(); return up; }
inline const Unpack& operator >> (const Unpack& up, Marshallable& m)	{ m.unmarshal(up); return up; }
//------------------------------------------- container -------------------------------------------------------------------------
template <typename T1, typename T2>
inline Pack& operator << (Pack& p, const std::pair<T1, T2>& t)
{
	p << t.first << t.second;
	return p;
}

template <typename K, typename V>
inline Pack& operator << (Pack& p, const std::map<K,V>& item)
{
	marshal_container16(p, item);
	return p;
}

template <typename T1, typename T2>
inline const Unpack& operator >> (const Unpack& up, std::pair<const T1, T2>& t)
{
	const T1& const_first = t.first;
	T1& _first = const_cast<T1&>(const_first);
	up >> _first >> t.second;
	return up;
}

template <typename T1, typename T2>
inline const Unpack& operator >> (const Unpack& up, std::pair<T1, T2>& t)
{
	up >> t.first >> t.second;
	return up;
}

template <typename K, typename V>
inline const Unpack& operator >> (const Unpack& up, std::map<K,V>& item)
{
	unmarshal_container16(up, std::inserter(item, item.begin()));
	return up;
}

template <typename Tcontainer>
inline void marshal_container16(Pack& p, const Tcontainer& c)
{
	p.push_uint16( uint16_t(c.size()) );
	for ( typename Tcontainer::const_iterator io = c.begin(); io != c.end(); ++io )
		p << *io;
}

template <typename OutputIterator>
inline void unmarshal_container16(const Unpack& up, OutputIterator io)
{
	for ( uint16_t idx = up.pop_uint16(); idx > 0; --idx )
	{
		typename OutputIterator::container_type::value_type tmp;
		up >> tmp;
		*io = tmp;
		++io;
	}
}

template <typename Tcontainer>
inline void marshal_container32(Pack& p, const Tcontainer& c)
{
	p.push_uint32( uint32_t(c.size()) );
	for ( typename Tcontainer::const_iterator io = c.begin(); io != c.end(); ++io )
		p << *io;
}

template <typename OutputIterator>
inline void unmarshal_container32(const Unpack& up, OutputIterator io)
{
	for ( uint32_t idx = up.pop_uint32(); idx > 0; --idx )
	{
		typename OutputIterator::container_type::value_type tmp;
		up >> tmp;
		*io = tmp;
		++io;
	}
}

