#pragma once
#include "ilinkhandler.h"
#include "fdbase.h"
#include <stdint.h>

struct ILink
	: public FdBase
{
	ILink() : m_ip(0), m_port(0), m_pHandler(NULL) {}
	virtual ~ILink() { m_ip = 0; m_port = 0, m_pHandler = NULL; } //it's virtual!!! virtual is very important!!!

	virtual int send(const char* data, size_t len, uint32_t ip=0, uint16_t port=0) = 0; //ip and port params are for udp used, tcp ignore it

	inline void set_handler(ILinkHandler* pH) { m_pHandler = pH; }
	inline uint32_t ip()	{ return m_ip; }
	inline uint16_t port()	{ return m_port; }

protected:
	uint32_t	m_ip;
	uint16_t	m_port;	
	ILinkHandler* m_pHandler;
};
