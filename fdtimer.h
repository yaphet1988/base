#pragma once
#include "fdbase.h"
#include <stdint.h>
#define _CLOCK_MODE_ CLOCK_REALTIME/*CLOCK_MONOTONIC*/  //Relatively time / ABS Real time
//if use CLOCK_MONOTONIC, the first 2nd timeout is inaccurate

struct ITimerCallback
{
	virtual void on_time(int id) = 0;
};

class FDTimer
	: public FdBase
{
public:
	FDTimer();
	~FDTimer();

 	virtual void on_socket_read();
	virtual void on_socket_write(){}
	virtual void on_socket_error(){}

public:
	void init(ITimerCallback* cb, int id = 1); //if multi timer in same class, distinguish with id
	void start(uint32_t sec, uint32_t msec = 0, bool bRepeat = true);
	void stop();
	bool is_running() { return _running; }

protected:
	ITimerCallback*	_cb;
	bool			_running;
	int				_id;
};

