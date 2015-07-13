#include "tcplink.h"
#include "env.h"
#include "comm.h"
#include "logger.h"
#include "ioengine.h"
#include "packbuf.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

TcpLink::TcpLink()
: m_bListen(false)
, m_bConnected(false)
{
	_fd_type = SOCK_TYPE_TCP;
	_fd = ::socket(AF_INET, SOCK_STREAM, 0);
}

TcpLink::TcpLink(int fd)
: m_bListen(false)
, m_bConnected(false)
{
	_fd_type = SOCK_TYPE_TCP;
	_fd = fd;
}

TcpLink::~TcpLink()
{
	Env::selector()->reg_event(this, IO_OPT_CLR);
	this->close();
}

void TcpLink::on_socket_read()
{
	if (!m_pHandler)
		return;

	if (m_bListen) //listen socket, accept it
	{
		m_pHandler->on_accept(this);
		return;
	}
	if (!m_bConnected) //onConnected
	{
		m_bConnected = true;
		m_pHandler->on_connected(this);
		return;
	}

	try
	{
		int read_res = m_input.read(_fd);
		if ( 0 < read_res )
		{
			int handle_len = m_pHandler->on_data(m_input.data(), m_input.size(), this);
			if ( 0 <= handle_len )
			{
				m_input.erase(0, handle_len);
			}
			else
			{
				//TO DO
			}
		}
		else
		{
			log(Debug, "tcp reset by peer, this:%p");
			this->on_socket_error();
		}
	}
	catch (UnpackError &se)
	{
		log(Error, "socket:%p UnpackError:%s", this, se.what() );
		this->on_socket_error();
	}
	catch(std::exception &ex) 
	{
		log(Error, "socket:%p Exception:%s", this, ex.what() );
		this->on_socket_error();
	}
}

void TcpLink::on_socket_write()
{
	if (!m_pHandler)
		return;

	if (!m_bConnected)
	{
		m_bConnected = true;
		m_pHandler->on_connected(this);
		return;
	}
	if ( m_output.flush(_fd) >=0 )
	{
		if ( m_output.empty() )
		{
			Env::selector()->reg_event(this, IO_OPT_DEL_WRITE);
		}
	}
	else
	{
		//TO DO
		this->on_socket_error();
		log(Error, "flush error! errno:%d",errno);
	}
}

void TcpLink::on_socket_error()
{
	log(Debug, "tcp on error! this:%p",this);
	if (m_pHandler)
	{
		m_pHandler->on_close(this);
	}
}

int TcpLink::connect(uint32_t ip, uint16_t port)
{
	m_ip = ip;
	m_port = port;
	set_non_block();

	sockaddr_in destAddr;
	destAddr.sin_family = AF_INET;
	destAddr.sin_addr.s_addr = ip;
	destAddr.sin_port = htons(port);
	Env::selector()->reg_event(this, IO_OPT_ADD_WRITE | IO_OPT_ADD_READ);

	if ( ::connect(_fd, (struct sockaddr*)&destAddr, sizeof(destAddr) ) == -1 )
	{
		if( EINPROGRESS == errno )
		{
			return 0; //still success
		}
		log(Warn, " connect failed, errno:%d",errno);
		this->on_socket_error();
		return errno;
	}
	return 0;
}

int TcpLink::send(const char* data, size_t len, uint32_t ip/* =0 */, uint16_t port/* =0 */)
{
	int ret = 0;
	if ( likely(m_bConnected) )
	{
		ret = m_output.write(_fd, data, len);
		if ( !m_output.empty() )
		{
			Env::selector()->reg_event(this, IO_OPT_ADD_WRITE);
		}
	}
	return ret;
}

int TcpLink::listen(uint16_t base_port)
{
	m_bListen = true;
#if 1 //linux, didn't work on bind same port but work on bind TIME_WAIT, only totally work in freeBSD
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
		if ( ::bind(_fd, (struct sockaddr *) &address, sizeof(address)) < 0 ) 
		{
			continue;
		}

		//bind success
		if( ::listen(_fd, SOMAXCONN) == -1 )
		{
			log(Error, "TcpSocket listen tcp socket port %u failed", m_port);
			close();
			return -1;
		}

		set_non_block();
		Env::selector()->reg_event(this, IO_OPT_ADD_READ);
		log(Info, "bind tcp socket port:%u successfully", m_port);
		return 0;
	}
	close();
	return -1;
}

int TcpLink::close()
{
	m_bConnected = false;
	::close(_fd);
	return 0;
}

TcpLink* TcpLink::accept()
{
	struct sockaddr_in sa;
	socklen_t len = sizeof(sa);
	int ret = ::accept(_fd, (struct sockaddr*)&sa, &len);
	if ( unlikely(0 >= ret) )
	{
		log(Error, "tcp can't accept! WTF, errno:%d",errno);
		return NULL;
	}

	TcpLink *pNew = new TcpLink(ret);
	//pNew->_fd = ret;
	pNew->m_ip = sa.sin_addr.s_addr;
	pNew->m_port = ntohs(sa.sin_port);
	pNew->m_bConnected = true;
	pNew->set_non_block();
	Env::selector()->reg_event(pNew, IO_OPT_ADD_READ);
	log(Info, "[TcpLink::Accept] this:%p new:%p ok",this,pNew);
	return pNew;
}

int TcpLink::set_non_block()
{
	int fflags = ::fcntl(_fd, F_GETFL);
	fflags |= O_NONBLOCK;
	::fcntl(_fd, F_SETFL, fflags);
	return 0;
}

