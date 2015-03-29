#include "env.h"
#include "uni.h"
#include "logger.h"
#include "ioengine.h"

IOEngine g_selector;

bool Env::init()
{
	uni::init_daemon();
	init_sys_log();
	return g_selector.init();
}

void Env::run()
{
	g_selector.run();
}

IOEngine* Env::selector()
{
	return &g_selector;
}

