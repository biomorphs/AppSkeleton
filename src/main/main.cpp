// Link against the engine libs
#ifdef SDE_DEBUG
	#pragma comment(lib,"../SDLEngine/bin/Debug/sdlengine.lib")
#else
	#pragma comment(lib,"../SDLEngine/bin/Release/sdlengine.lib")
#endif

#include "engine/engine_startup.h"

class SystemRegistration : public Engine::ISystemRegistrar
{
public:
	void RegisterSystems(Core::SystemManager& systemManager)
	{

	}
};

int main(int argc, char** argv)
{
	SystemRegistration sysRegistration;
	return Engine::Run(sysRegistration, argc, argv);
}