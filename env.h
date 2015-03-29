#pragma once

class IOEngine;
class Env
{
public:
	static bool init();
	static void run();

	static IOEngine* selector(); //don't touch it only if u know what's going on!
};

