#include "websocklink.h"
#include "env.h"
#include "uni.h"
#include "comm.h"
#include "logger.h"
#include "ioengine.h"
#include "packbuf.h"
#include "encrypt.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define HTTP_HEADER_MAX_LEN 8192
#define FRAME_HEADER_MIN_LEN 2 //16 bits
#define MASKING_KEY_LEN 4 //32 bits
#define WEBSOCK_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define __CLASS__ "WebSockLink"

enum
{
	OPCODE_CONTINUE = 0x0,
	OPCODE_TEXT		= 0x1,
	OPCODE_BINARY	= 0x2,
	OPCODE_CLOSE	= 0x8,
	OPCODE_PING		= 0x9,
	OPCODE_PONG		= 0xA
};

/*********************** websocket frame format ********************
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-------+-+-------------+-------------------------------+
 |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 | |1|2|3|       |K|             |                               |
 +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 |     Extended payload length continued, if payload len == 127  |
 + - - - - - - - - - - - - - - - +-------------------------------+
 |                               |Masking-key, if MASK set to 1  |
 +-------------------------------+-------------------------------+
 | Masking-key (continued)       |          Payload Data         |
 +-------------------------------- - - - - - - - - - - - - - - - +
 :                     Payload Data continued ...                :
 + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 |                     Payload Data continued ...                |
 +---------------------------------------------------------------+
*******************************************************************/

WebSockLink::WebSockLink()
: m_bListen(false)
, m_bConnected(false)
, m_bHandshakeDone(false)
{
	_fd_type = SOCK_TYPE_WEBSOCK;
	_fd = ::socket(AF_INET, SOCK_STREAM, 0);
}

WebSockLink::~WebSockLink()
{
	Env::selector()->reg_event(this, IO_OPT_CLR);
	this->close();
}

void WebSockLink::on_socket_read()
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
		return;
	}

	try
	{
		int read_res = m_raw_input.read(_fd);
		if ( 0 < read_res )
		{
			int handle_len = pre_handle_data(m_raw_input.data(), m_raw_input.size());
			if ( 0 <= handle_len )
			{
				m_raw_input.erase(0, handle_len);
			}
			else
			{
				this->on_socket_error();
			}
		}
		else
		{
			FUNLOG(Info, " link reset by peer, this:%p", this);
			this->on_socket_error();
		}
	}
	catch (UnpackError &se)
	{
		FUNLOG(Error, " socket:%p UnpackError:%s", this, se.what() );
		this->on_socket_error();
	}
	catch(std::exception &ex) 
	{
		FUNLOG(Error, " socket:%p Exception:%s", this, ex.what() );
		this->on_socket_error();
	}
}

void WebSockLink::on_socket_write()
{
	if (!m_pHandler)
		return;

	if (!m_bConnected)
	{
		m_bConnected = true;
		return;
	}
	if ( m_output.flush(_fd) >=0 )
	{
		if ( m_output.empty() )
		{
			Env::selector()->reg_event(this, IO_OPT_DEL_WRITE);
		}
	}
}

void WebSockLink::on_socket_error()
{
	log(Debug, "tcp on error! this:%p",this);
	if (m_pHandler)
	{
		m_pHandler->on_close(this);
	}
}

int WebSockLink::handshake(uint32_t ip, uint16_t port)
{
	return 0;
}

int WebSockLink::send(const char* data, size_t len, uint32_t ip/* =0 */, uint16_t port/* =0 */)
{
	int ret = 0;
	if ( likely(m_bHandshakeDone) )
	{
		//no MASK when server send to client
		Pack _pk; //use body field to do a tmp buffer
		uint8_t _FIN = 1;
		_pk.push_uint8( (_FIN << 7) | OPCODE_BINARY );
		if ( len <= 125 ) //refer to RFC-6455 (websocket protocol frame formate)
		{
			_pk.push_uint8(len);
		}
		else if ( len <= ((1 << 16 )-1) ) //16 bits length
		{
			_pk.push_uint8(126);
			_pk.push_uint16(len);
		}
		else //64 bits length
		{
			_pk.push_uint8(127);
			_pk.push_uint64(len);
		}
		_pk.push(data, len); //can't use push_strxx
		ret = m_output.write(_fd, _pk.body(), _pk.body_size()); //buffer only body, not include header
		std::string _print(data, len);
		if ( !m_output.empty() )
		{
			Env::selector()->reg_event(this, IO_OPT_ADD_WRITE);
		}
	}
	return ret;
}

int WebSockLink::listen(uint16_t base_port)
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
		address.sin_port = htons(m_port);
		if ( ::bind(_fd, (struct sockaddr *) &address, sizeof(address)) < 0 ) 
		{
			continue;
		}
		if( ::listen(_fd, SOMAXCONN) == -1 )
		{
			FUNLOG(Error, " listen socket port %u failed", m_port);
			close();
			return -1;
		}

		set_non_block();
		Env::selector()->reg_event(this, IO_OPT_ADD_READ);
		FUNLOG(Info, "bind socket port:%u successfully", m_port);
		return 0;
	}
	close();
	return -1;
}

int WebSockLink::close()
{
	m_bConnected = false;
	::close(_fd);
	return 0;
}

WebSockLink* WebSockLink::accept()
{
	struct sockaddr_in sa;
	socklen_t len = sizeof(sa);
	int ret = ::accept(_fd, (struct sockaddr*)&sa, &len);
	
	WebSockLink *pNew = new WebSockLink();
	pNew->_fd = ret;
	pNew->m_ip = sa.sin_addr.s_addr;
	pNew->m_port = ntohs(sa.sin_port);
	pNew->m_bConnected = true;
	pNew->set_non_block();
	Env::selector()->reg_event(pNew, IO_OPT_ADD_READ);
	FUNLOG(Info, "this:%p new:%p ok", this, pNew);
	return pNew;
}

int WebSockLink::set_non_block()
{
	int fflags = ::fcntl(_fd, F_GETFL);
	fflags |= O_NONBLOCK;
	::fcntl(_fd, F_SETFL, fflags);
	return 0;
}

int WebSockLink::pre_handle_data(const char* data, size_t len)
{
	if (m_bHandshakeDone)
		return handle_frame_data(data, len);
	else
		return handle_handshake(data, len);
}

int WebSockLink::handle_handshake(const char* data, size_t len)
{
	if ( len > HTTP_HEADER_MAX_LEN )
	{
		FUNLOG(Error, " too large http header! len:%u fromip:%s", len, uni::addr_ntoa(m_ip).c_str());
		return -1;
	}
	std::string _http_header(data, len);
	std::string::size_type _httphdend;
	if ( (_httphdend = _http_header.find("\r\n\r\n")) == std::string::npos ) //header not finish, wait to receive the rest of it
		return 0;
	
	std::string::size_type _keyhdstart;
	if ( _http_header.find("Upgrade: websocket\r\n") == std::string::npos ||
         _http_header.find("Connection: Upgrade\r\n") == std::string::npos ||
        (_keyhdstart = _http_header.find("Sec-WebSocket-Key: ")) == std::string::npos
		)
	{
		FUNLOG(Error, " missing required header fields! fromip:%s", uni::addr_ntoa(m_ip).c_str());
		return -1;
	}
	_keyhdstart += 19;
	std::string::size_type _keyhdend = _http_header.find("\r\n", _keyhdstart);
	m_client_key = _http_header.substr(_keyhdstart, _keyhdend-_keyhdstart);
	m_accept_key = create_acceptkey( uni::trim(m_client_key) );
	respone_handshake(m_accept_key);
	m_bHandshakeDone = true;
	return _httphdend+4;
}

/*******************************************************************
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
|                               |Masking-key, if MASK set to 1  |
+-------------------------------+-------------------------------+
| Masking-key (continued)       |          Payload Data         |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+
**********************************************************************/

int WebSockLink::handle_frame_data(const char* data, size_t len)
{
	int _handle_len = 0;
	while ( len >= FRAME_HEADER_MIN_LEN )
	{
		size_t _total_frame_len = FRAME_HEADER_MIN_LEN;
		Unpack _up(data, len);
		uint16_t _frame_header = _up.pop_uint16();
		uint8_t _MASK = (_frame_header & 0x0080) >> 7;
		uint8_t _payload_len = (_frame_header & 0x007F);
		if (!!_MASK)
			_total_frame_len += MASKING_KEY_LEN;
		if ( _payload_len <= 125 )
		{
				_total_frame_len += _payload_len;
		}
		else if ( _payload_len == 126 )
		{
			_total_frame_len += 2; //2 bytes(16 bits) length
			if ( len < _total_frame_len ) //not enough data
				break;
			_payload_len = _up.pop_uint16();
			_total_frame_len += _payload_len;
		}
		else if ( _payload_len == 127 )
		{
			_total_frame_len += 8; //8 bytes(64 bits) length
			if ( len < _total_frame_len ) //not enough data
				break;
			_payload_len = _up.pop_uint64();
			_total_frame_len += _payload_len;
		}
		if ( len < _total_frame_len ) //not enough data
			break;

		//data enough for a frame, start to decode it
		//decode_frame(data+_handle_len, _total_frame_len);
		//uint8_t _FIN = (_frame_header & 0x8000) >> 15; //useless, I donn't need to know cause I have app_input cache!
		uint8_t _opcode = (_frame_header >> 8) & 0x000F;
		switch (_opcode)
		{
		case OPCODE_PING:
			{
				FUNLOG(Info, " ping %p", this);
				Pack _pk; //use body field to do a tmp stream
				uint8_t _FIN = 1;
				_pk.push_uint8( (_FIN << 7) | OPCODE_PONG );
				_pk.push_uint8(0); //len is 0
				send(_pk.body(), _pk.body_size());
				break;
			}
		case OPCODE_CLOSE:
			{
				FUNLOG(Info, " webcosket reset by peer(CLOSE frame was receivered %p)", this);
				m_pHandler->on_close(this);
				break;
			}
		case OPCODE_TEXT:
		case OPCODE_BINARY:
			{
				uint8_t _Masking_key_array[4];
        		if (!!_MASK)
        		{
            		for ( int i = 0; i < 4; ++i )
            		{
                		_Masking_key_array[i] = _up.pop_uint8();
            		}
        		}
        		for ( size_t i = 0; i < _payload_len; ++i )
        		{
            		uint8_t _tmp = (*(uint8_t*)(_up.data()+i) ^ _Masking_key_array[i%4]);
            		m_app_input.append( (const char*)(&_tmp), 1);
        		}
				int _app_handle_len = m_pHandler->on_data(m_app_input.data(), m_app_input.size(), this); //handle this app data to app layer
				if ( 0 <= _app_handle_len )
					m_app_input.erase(0, _app_handle_len);
				break;
			}
		default:
			FUNLOG(Info, " unknown opcode:%u", _opcode);
		}
		_handle_len += _total_frame_len;
        len -= _total_frame_len;
	}
	return _handle_len;
}

int WebSockLink::respone_handshake(const std::string& acceptkey)
{
	std::string _res_http_header;
	_res_http_header.assign("HTTP/1.1 101 Switching Protocols\r\n");
	_res_http_header.append("Upgrade:websocket\r\n");
	_res_http_header.append("Connection:Upgrade\r\n");
	_res_http_header.append("Sec-WebSocket-Accept:").append(acceptkey).append("\r\n");
	_res_http_header.append("\r\n");
	int ret = m_output.write(_fd, _res_http_header.data(), _res_http_header.size());
	if ( !m_output.empty() )
		Env::selector()->reg_event(this, IO_OPT_ADD_WRITE);
	FUNLOG(Info, " client_key:%s accept_key:%s", m_client_key.data(), m_accept_key.data());
	return ret;
}

std::string WebSockLink::create_acceptkey(const std::string& clientkey)
{
	std::string s = clientkey + WEBSOCK_GUID;
	return encdec::base64_encode( encdec::sha1(s) );
}
