#include "engine/engine_startup.h"
#include "engine/input_system.h"
#include "core/system_registrar.h"
#include "sde/render_system.h"
#include "app_skeleton.h"

// Register the app systems here
class SystemRegistration : public Engine::IAppSystemRegistrar
{
public:
	void RegisterSystems(Core::ISystemRegistrar& systemManager)
	{
		systemManager.RegisterSystem("Input", new Engine::InputSystem());
		systemManager.RegisterSystem("App", new AppSkeleton());
		systemManager.RegisterSystem("Render", new SDE::RenderSystem());
	}
};

int main(int argc, char** argv)
{
	SystemRegistration sysRegistration;
	return Engine::Run(sysRegistration, argc, argv);
}