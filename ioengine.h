#pragma once
#include <stdint.h>
#include <set>

struct FdBase;
class IOEngine
{
public:
	IOEngine();
	~IOEngine();

public:
	bool init();
	void run();
	void reg_event(FdBase* pfd, int opt);

private:
	void _on_event_read(FdBase* pfd);
	void _on_event_write(FdBase* pfd);
	void _on_event_error(FdBase* pfd);
	bool _is_alive(FdBase* pfd);

private:
	typedef std::set<FdBase*> fd_set_t;
	int			m_hEPoll;
	bool		m_bRunning;
	fd_set_t	m_fds;
};

