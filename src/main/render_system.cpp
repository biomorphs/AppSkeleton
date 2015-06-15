#include "render_system.h"
#include "render/window.h"
#include "render/device.h"
#include "kernel/assert.h"

RenderSystem::RenderSystem()
: m_quit(false)
, m_window(nullptr)
, m_device(nullptr)
{
}

bool RenderSystem::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	m_window = new Render::Window(Render::Window::Properties("Skeleton Application", 640, 480));
	m_device = new Render::Device(*m_window);
	m_window->Show();

	m_posBuffer.Create(9 * sizeof(float));

	const float c_triangle[] = {
		0.0f, 0.5f, 0.0f,
		-1.0f, -0.5f, 0.0f,
		1.0f, -0.5f, 0.0f
	};
	m_posBuffer.SetData(0, sizeof(c_triangle), (void*)c_triangle);

	m_vertexArray.AddBuffer(0, &m_posBuffer, Render::VertexDataType::Float, 3);
	m_vertexArray.Create();

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
	m_vertexArray.Destroy();
	m_posBuffer.Destroy();
	m_window->Hide();
	delete m_device;
	delete m_window;
}