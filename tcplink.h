#pragma once
#include "ilink.h"
#include "sockbuf.h"

class TcpLink
	: public ILink
{
public:
	TcpLink();
private:
	TcpLink(int fd); //for accept calls
public:
	~TcpLink();

	virtual void on_socket_read();
	virtual void on_socket_write();
	virtual void on_socket_error();

	virtual int send(const char* data, size_t len, uint32_t ip=0, uint16_t port=0);

public:
	int connect(uint32_t ip, uint16_t port);
	int listen(uint16_t base_port);
	int set_non_block();
	int close();
	TcpLink* accept();

private:
	bool		m_bListen;
	bool		m_bConnected;
	inputbuf_t	m_input;
	outputbuf_t m_output;
};

