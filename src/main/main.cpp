#include "sde/asset_system.h"
#include "sde/render_system.h"
#include "sde/job_system.h"
#include "debug_gui/debug_gui_system.h"
#include "engine/engine_startup.h"
#include "input/input_system.h"
#include "core/system_registrar.h"
#include "app_skeleton.h"

// Register the app systems here
class SystemRegistration : public Engine::IAppSystemRegistrar
{
public:
	void RegisterSystems(Core::ISystemRegistrar& systemManager)
	{
		systemManager.RegisterSystem("Jobs", new SDE::JobSystem());
		systemManager.RegisterSystem("Input", new Input::InputSystem());
		systemManager.RegisterSystem("Assets", new SDE::AssetSystem());
		systemManager.RegisterSystem("App", new AppSkeleton());
		systemManager.RegisterSystem("DebugGui", new DebugGui::DebugGuiSystem());
		systemManager.RegisterSystem("Render", new SDE::RenderSystem());
	}
};

int main(int argc, char** argv)
{
	SystemRegistration sysRegistration;
	return Engine::Run(sysRegistration, argc, argv);
}