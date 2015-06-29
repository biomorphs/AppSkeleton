#include "engine/engine_startup.h"
#include "engine/input_system.h"
#include "core/system_registrar.h"
#include "render_system.h"

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