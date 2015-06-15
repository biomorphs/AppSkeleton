#pragma once

#include "core/system.h"
#include "core/system_enumerator.h"
#include "core/event_listener.h"
#include "render/window.h"
#include "render/device.h"
#include "render/render_buffer.h"
#include "render/vertex_array.h"

class RenderSystem : public Core::ISystem
{
public:
	RenderSystem();
	bool PreInit(Core::ISystemEnumerator& systemEnumerator);
	bool Tick();
	void Shutdown();

private:
	void OnEventRecieved(const Core::EngineEvent& e);
	bool m_quit;

	Render::Window* m_window;
	Render::Device* m_device;
	Render::RenderBuffer m_posBuffer;
	Render::VertexArray m_vertexArray;
};