#pragma once

#include "core/system.h"
#include "core/system_enumerator.h"
#include "framework/event_listener.h"
#include "render/window.h"
#include "render/device.h"

class RenderSystem : public Core::ISystem, public Framework::IEventListener
{
public:
	RenderSystem();
	bool PreInit(Core::ISystemEnumerator& systemEnumerator);
	bool Initialise();
	bool Tick();
	void Shutdown();

private:
	void OnEventRecieved(const SDL_Event& e);

	bool m_quit;
	Render::Window* m_window;
	Render::Device* m_device;
};