#pragma once

#include "engine/engine_startup.h"
#include "framework/event_system.h"
#include "core/system_manager.h"
#include "render_system.h"

class SystemRegistration : public Engine::ISystemRegistrar
{
public:
	void RegisterSystems(Core::SystemManager& systemManager)
	{
		systemManager.AddSystem<Framework::EventSystem>("Events");
		systemManager.AddSystem<RenderSystem>("Render");
	}
};