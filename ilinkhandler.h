#pragma once
#include <stdint.h>
#include <unistd.h>

struct ILink;
struct ILinkHandler
{
	virtual void on_connected(ILink* pLink) {}
	virtual void on_close(ILink* pLink) {}
	virtual int  on_data(const char* data, size_t len, ILink* pLink) { return len; } //return the length that handle
	virtual void on_accept(ILink* pLink) {} //listen socket
};
