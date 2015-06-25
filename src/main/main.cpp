#include "engine/engine_startup.h"
#include "engine/input_system.h"
#include "core/system_registrar.h"
#include "render_system.h"

// Link against the engine
#ifdef SDE_DEBUG
	#pragma comment(lib,"../SDLEngine/bin/Debug/sdlengine.lib")
#else
	#pragma comment(lib,"../SDLEngine/bin/Release/sdlengine.lib")
#endif

// Register the app systems here
class SystemRegistration : public Engine::IAppSystemRegistrar
{
public:
	void RegisterSystems(Core::ISystemRegistrar& systemManager)
	{
		systemManager.RegisterSystem("Input", new Engine::InputSystem());
		systemManager.RegisterSystem("Render", new RenderSystem());
	}
};

int main(int argc, char** argv)
{
	SystemRegistration sysRegistration;
	return Engine::Run(sysRegistration, argc, argv);
}