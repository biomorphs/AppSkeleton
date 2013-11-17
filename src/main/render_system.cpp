#include "render_system.h"
#include "render/window.h"
#include "render/device.h"
#include "kernel/assert.h"
#include "kernel/refcounted_data.h"
#include "kernel/handle.h"
#include "kernel/object_bucket.h"

RenderSystem::RenderSystem()
: m_quit(false)
, m_window(nullptr)
, m_device(nullptr)
, m_textureCache(nullptr)
{
}

bool RenderSystem::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	m_window = new Render::Window(Render::Window::Properties("Skeleton Application", 640, 480));
	m_device = new Render::Device(*m_window);
	m_textureCache = new Render::TextureCache(m_device);
	m_window->Show();

	return true;
}

void RenderSystem::OnEventRecieved(const Core::EngineEvent& e)
{
	if (e.m_type == Core::EngineEvent::QuitRequest)
	{
		m_quit = true;
	}
}

bool RenderSystem::Tick()
{
	m_device->Present();
	return !m_quit;
}

void RenderSystem::Shutdown()
{
	m_window->Hide();
	delete m_textureCache;
	delete m_device;
	delete m_window;
}