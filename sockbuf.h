#pragma once
#include "logger.h"
#include "filter.h"
#include "blockbuf.h"

typedef int SOCKET;

template<typename BufferT, typename FilterT = FilterBase>
struct SockBuf
	: public BufferT
	, public FilterT
{
	int read(SOCKET s)
	{
		if ( BufferT::freespace() < (BufferT::blocksize() >> 1) 
			&& BufferT::blocknum() < BufferT::mMAXBLOCKNUM )
			// ignore increase_capacity result.
			BufferT::increase_capacity( BufferT::blocksize() );

		size_t nrecv = BufferT::freespace() < BufferT::mPos ? BufferT::freespace() : BufferT::mPos;  // min(mPos, freespace());
		if (nrecv == 0) 
			return -1;

		int	ret = ::recv(s, (char*)BufferT::tail(), (int)nrecv, 0);
		if (ret > 0)
			BufferT::m_size += ret;
		else
		{
			//log(Warn, "read nothing, link may be broken, errno:%d",errno);
		}
		return ret;
	}

	int write(SOCKET s, const char* data, size_t len)
	{
		if (len == 0) 
			return -1;

		if( BufferT::blocknum() > BufferT::mMAXBLOCKNUM ) 
			return -1;

		int nsent = 0;
		if ( BufferT::empty() ) //call send as no data cached in buffer,otherwise,socket can't send anything and we should cache the data into buffer until onSend event was given
		{
			nsent = ::send(s , data, (int)len, 0);
		}

		if (nsent < 0)
		{
			if(errno == EAGAIN || errno == EINTR || errno == EINPROGRESS)
				nsent = 0;
			else
			{
				log(Error, " send error! errno:%d",errno);
				//throw buffer_overflow("send error");
			}
		}

		if ( !BufferT::append(data + nsent, len - nsent) )
		{
			log(Error, " append failed!!! send data len:%u",len);
		}
		return (int)nsent;
	}

	int flush(SOCKET s)
	{
		if ( BufferT::empty() )
		{
			return 0;
		}

		int ret = ::send(s, (const char*)BufferT::data(), (int)BufferT::size(), 0);
		if ( 0 < ret ) //always true! in case
			BufferT::erase( 0, ret );
		//FUNLOG(" sent bytes=",ret);
		return ret;
	}
};

typedef SockBuf<Buffer128x64k, FilterRC4>	inputbuf_t; //8M
typedef SockBuf<Buffer128x64k, FilterRC4>	outputbuf_t; //8M, increase output buffer max size limit, in case buffer overflow in swarming (send data)

