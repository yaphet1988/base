#include "fdtimer.h"
#include "env.h"
#include "comm.h"
#include "logger.h"
#include "ioengine.h"
#include <sys/timerfd.h>

FDTimer::FDTimer()
: _running(false)
{
	_fd = ::timerfd_create(_CLOCK_MODE_, TFD_NONBLOCK);
	_fd_type = SOCK_TYPE_TIMER;
}

FDTimer::~FDTimer()
{
	Env::selector()->reg_event(this, IO_OPT_CLR);
	::close(_fd);
}

void FDTimer::on_socket_read()
{
	long long buf;
	::read( _fd, &buf, sizeof(buf) ); //it didn't mean i wanna read something
	if ( likely( NULL != _cb ) )
	{
		_cb->on_time(_id);
	}
}

void FDTimer::init(ITimerCallback* cb, int id /* = 1 */)
{
	_cb = cb;
	_id = id;
}

void FDTimer::start(uint32_t sec, uint32_t msec /* = 0 */, bool bRepeat /* = true */)
{
	struct itimerspec newtime;
	newtime.it_value.tv_sec = sec;
	newtime.it_value.tv_nsec = msec * 1000 * 1000;
	if (bRepeat)
	{
		newtime.it_interval.tv_sec = sec;
		newtime.it_interval.tv_nsec = msec * 1000 * 1000;
	}
	::timerfd_settime(_fd, _CLOCK_MODE_, &newtime, NULL);

	if (!_running)
	{
		Env::selector()->reg_event(this, IO_OPT_ADD_READ);
		_running = true;
	}
}

void FDTimer::stop()
{
	_running = false;
	struct itimerspec newtime;
	newtime.it_value.tv_sec = 0;
	newtime.it_value.tv_nsec = 0;
	newtime.it_interval.tv_sec = 0;
	newtime.it_interval.tv_nsec = 0;
	::timerfd_settime(_fd, _CLOCK_MODE_, &newtime, NULL);
}

