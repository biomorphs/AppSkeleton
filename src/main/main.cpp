#include "system_registration.h"

// Link against the engine
#ifdef SDE_DEBUG
	#pragma comment(lib,"../SDLEngine/bin/Debug/sdlengine.lib")
#else
	#pragma comment(lib,"../SDLEngine/bin/Release/sdlengine.lib")
#endif

int main(int argc, char** argv)
{
	SystemRegistration sysRegistration;
	return Engine::Run(sysRegistration, argc, argv);
}