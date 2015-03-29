#pragma once
#include <unistd.h>

enum
{
	SOCK_TYPE_TCP = 1,
	SOCK_TYPE_UDP = 2,
	SOCK_TYPE_WEBSOCK = 3,
	SOCK_TYPE_TIMER = 4 //timerfd
};

struct FdBase
{
	virtual void on_socket_read() = 0;
	virtual void on_socket_write() = 0;
	virtual void on_socket_error() = 0;

	inline int fd()			{ return _fd; }
	inline int fd_type()	{ return _fd_type; }

protected:
	int _fd;
	int _fd_type;
};

