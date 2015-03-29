#include "udplink.h"
#include "env.h"
#include "comm.h"
#include "ioengine.h"
#include "logger.h"
#include "packbuf.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

UdpLink::UdpLink()
{
	_fd_type = SOCK_TYPE_UDP;
	_fd = ::socket(AF_INET , SOCK_DGRAM, 0);
	Env::selector()->reg_event(this, IO_OPT_ADD_READ);
}

UdpLink::~UdpLink()
{
	Env::selector()->reg_event(this, IO_OPT_CLR);
	this->close();
}

void UdpLink::on_socket_read()
{
	if (!m_pHandler)
		return;
	try
	{
		struct sockaddr_in _sock_from;
		memset(&_sock_from, 0, sizeof(sockaddr_in));
		socklen_t	_sock_len = sizeof(_sock_from);
		int _read_len = ::recvfrom(_fd, m_readBuf, sizeof(m_readBuf), 0, (sockaddr*)(&_sock_from), &_sock_len);
		if ( likely(0 < _read_len) )
		{
			this->m_ip = _sock_from.sin_addr.s_addr;
			this->m_port = htons(_sock_from.sin_port);
			int _handle_len = m_pHandler->on_data(m_readBuf, _read_len, this);
			if ( unlikely(_handle_len < 0) )
			{
				this->close();
			}
		}
		else
		{
			log(Warn, "(UDP) recvfrom read nothing, this:%p");
		}
	}
	catch (UnpackError &se)
	{
		log(Error, "(UDP)socket:%p UnpackError:%s", this, se.what() );
		this->on_socket_error();
	}
	catch(std::exception &ex) 
	{
		log(Error, "(UDP)socket:%p Exception:%s", this, ex.what() );
		this->on_socket_error();
	}
}

void UdpLink::on_socket_error()
{

}

int UdpLink::send(const char* data, size_t len, uint32_t ip/* =0 */, uint16_t port/* =0 */)
{
	struct sockaddr_in _dest;
	memset(&_dest, 0, sizeof(sockaddr_in));
	_dest.sin_family		= AF_INET;
	_dest.sin_addr.s_addr	= ip;
	_dest.sin_port			= htons(port);

	int _sent_num = ::sendto(_fd, data, len, 0, (struct sockaddr*)&_dest, sizeof(sockaddr));
	if ( unlikely(_sent_num < 0) )
	{
		log(Error, " sendto fail! _sent_num:%d",_sent_num);
	}
	return _sent_num;
}

int UdpLink::listen(uint16_t base_port)
{
#if 0 //linux, forbid bind same port! or only one process can recv data!
	int op = 1;
	if (-1 == setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)))
	{
		close();
		return -1;
	}
#endif

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	for ( int i = 0; i < 1000; i++ )
	{
		m_port = base_port + i;
		if( m_port == 445 )
		{
			// 445 is the samba port, blocked by firewall
			continue;
		}
		address.sin_port = htons(m_port);
		if ( ::bind(_fd, (struct sockaddr *) &address, sizeof(address)) != 0 ) 
		{
			continue;
		}
		//set_non_block(); //make udp block mode!
		//Env::selector()->reg_event(this, IO_OPT_ADD_READ);
		log(Info, "bind udp socket port:%u successfully", m_port);
		return 0;
	}
	return -1;
}

int UdpLink::set_non_block()
{
	int fflags = ::fcntl(_fd, F_GETFL);
	fflags |= O_NONBLOCK;
	::fcntl(_fd, F_SETFL, fflags);
	return 0;
}

int UdpLink::close()
{
	::close(_fd);
	return 0;
}

