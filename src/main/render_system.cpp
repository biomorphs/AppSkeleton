#include "render_system.h"
#include "framework/event_system.h"
#include "render/window.h"
#include "render/device.h"

RenderSystem::RenderSystem()
: m_quit(false)
{

}

bool RenderSystem::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	Framework::EventSystem* eventSys = static_cast<Framework::EventSystem*>(systemEnumerator.GetSystem("Events"));
	eventSys->RegisterListener(this);

	return true;
}

void RenderSystem::OnEventRecieved(const SDL_Event& e)
{
	if (e.type == SDL_QUIT)
	{
		m_quit = true;
	}
}

bool RenderSystem::Initialise()
{
	m_window = new Render::Window(Render::Window::Properties("Skeleton Application", 640, 480));
	m_device = new Render::Device(*m_window);
	m_window->Show();
	return true;
}

bool RenderSystem::Tick()
{
	m_device->Clear(128, 128, 255, 255);
	m_device->Present();
	return !m_quit;
}

void RenderSystem::Shutdown()
{
	m_window->Hide();
	delete m_device;
	delete m_window;
}