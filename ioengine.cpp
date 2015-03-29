#include "ioengine.h"
#include "fdbase.h"
#include "comm.h"
#include "logger.h"

#include <sys/epoll.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#define EPOLL_SIZE 100

static int pipe_err = 0;

static void epoll_pipe_handler(int)
{
	pipe_err++;
}

IOEngine::IOEngine()
{
}

IOEngine::~IOEngine()
{
	::close(m_hEPoll);
}

bool IOEngine::init()
{
	if ( signal(SIGPIPE, epoll_pipe_handler) == SIG_ERR )
	{
		//log
		return false;
	}

	m_hEPoll = ::epoll_create(65535);
	if (-1 == m_hEPoll)
		return false;

	m_bRunning = true;
	return true;
}

void IOEngine::reg_event(FdBase* pfd, int opt) //epoll run at LT mode, not ET mode here
{
	epoll_event ev;
	ev.data.ptr = pfd;
	ev.events = EPOLLIN;
	
	if ( opt & IO_OPT_CLR )
	{
		m_fds.erase(pfd);
		::epoll_ctl(m_hEPoll, EPOLL_CTL_DEL, pfd->fd(), &ev);
	}
	else
	{
		if ( opt & IO_OPT_ADD_READ )
			ev.events |= EPOLLIN;
		if ( opt & IO_OPT_ADD_WRITE )
			ev.events |= EPOLLOUT;
		if ( opt & IO_OPT_DEL_READ )
			ev.events &= ~EPOLLIN;
		if ( opt & IO_OPT_DEL_WRITE )
			ev.events &= ~EPOLLOUT;

		if ( m_fds.find(pfd) == m_fds.end() )
		{
			m_fds.insert(pfd);
			::epoll_ctl(m_hEPoll, EPOLL_CTL_ADD, pfd->fd(), &ev);
		}
		else
		{
			::epoll_ctl(m_hEPoll, EPOLL_CTL_MOD, pfd->fd(), &ev);
		}
	}
}

void IOEngine::run()
{
	epoll_event events[EPOLL_SIZE];

	while (m_bRunning)
	{
		int iActive = ::epoll_wait(m_hEPoll, events, EPOLL_SIZE, 50);
		if (iActive < 0) //zero means timeout
		{
			if ( EINTR == errno )
				continue;
			else
				log(Error, "epoll run error! errno:%u",errno);
		}

		for ( int i = 0; i < iActive; ++i )
		{
			FdBase *pfd = (FdBase*)(events[i].data.ptr);

			if ( (events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) )
			{
				_on_event_error(pfd);
				continue;
			}
			if ( events[i].events & EPOLLIN )
			{
				_on_event_read(pfd);
			}
			if ( events[i].events & EPOLLOUT )
			{
				_on_event_write(pfd);
			}
		}
		//clearRemoved();
	}
}

void IOEngine::_on_event_read(FdBase* pfd)
{
	if ( pfd && _is_alive(pfd) )
		pfd->on_socket_read();
}

void IOEngine::_on_event_write(FdBase* pfd)
{
	if ( pfd && _is_alive(pfd) )
		pfd->on_socket_write();
}

void IOEngine::_on_event_error(FdBase* pfd)
{
	if ( pfd && _is_alive(pfd) )
		pfd->on_socket_error();
}

bool IOEngine::_is_alive(FdBase* pfd)
{
	return m_fds.end() != m_fds.find(pfd);
}

