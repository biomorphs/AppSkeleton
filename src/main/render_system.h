#pragma once

#include "core/system.h"
#include "core/system_enumerator.h"
#include "core/event_listener.h"
#include "render/window.h"
#include "render/device.h"
#include "render/render_buffer.h"
#include "render/vertex_array.h"
#include "render/shader_binary.h"
#include "render/shader_program.h"

class RenderSystem : public Core::ISystem
{
public:
	RenderSystem();
	virtual ~RenderSystem();
	bool PreInit(Core::ISystemEnumerator& systemEnumerator);
	bool Tick();
	void Shutdown();

private:
	void OnEventRecieved(const Core::EngineEvent& e);
	bool m_quit;

	bool LoadShaders();
	bool CreateMesh();

	Render::Window* m_window;
	Render::Device* m_device;
	Render::RenderBuffer m_posBuffer;
	Render::RenderBuffer m_colourBuffer;
	Render::VertexArray m_vertexArray;
	Render::ShaderBinary m_vertexShader;
	Render::ShaderBinary m_fragmentShader;
	Render::ShaderProgram m_shaderProgram;
};