#pragma once
#include "ilink.h"
#define UDP_READ_BUF_SIZE 1024*1024 //1MB!

class UdpLink
	: public ILink
{
public:
	UdpLink();
	~UdpLink();

	virtual void on_socket_read();
	virtual void on_socket_write() {}
	virtual void on_socket_error();

	virtual int send(const char* data, size_t len, uint32_t ip=0, uint16_t port=0);

public:
	int listen(uint16_t base_port);
	int set_non_block();
	int close();

private:
	char		m_readBuf[UDP_READ_BUF_SIZE];
};

