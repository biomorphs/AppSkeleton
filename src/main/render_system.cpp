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

RenderSystem::~RenderSystem()
{
}

bool RenderSystem::LoadShaders()
{
	std::string compileResult;
	if (!m_vertexShader.CompileFromFile(Render::ShaderType::VertexShader, "simple_vertex.txt", compileResult))
	{
		SDE_LOG("Failed to compile shader:\r\n\t%s", compileResult.c_str());
		return false;
	}

	if (!m_fragmentShader.CompileFromFile(Render::ShaderType::FragmentShader, "simple_fragment.txt", compileResult))
	{
		SDE_LOG("Failed to compile shader:\r\n\t%s", compileResult.c_str());
		return false;
	}

	if (!m_shaderProgram.Create(m_vertexShader, m_fragmentShader, compileResult))
	{
		SDE_LOG("Failed to link shaders: \r\n\t%s", compileResult.c_str());
		return false;
	}

	return true;
}

bool RenderSystem::CreateMesh()
{
	m_posBuffer.Create(9 * sizeof(float));
	m_colourBuffer.Create(9 * sizeof(float));

	const float c_triangle[] = {
		0.0f, 0.5f, 0.0f,
		-1.0f, -0.5f, 0.0f,
		1.0f, -0.5f, 0.0f
	};
	const float c_colour[] = {
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f
	};

	m_posBuffer.SetData(0, sizeof(c_triangle), (void*)c_triangle);
	m_colourBuffer.SetData(0, sizeof(c_colour), (void*)c_colour);

	m_vertexArray.AddBuffer(0, &m_posBuffer, Render::VertexDataType::Float, 3);
	m_vertexArray.AddBuffer(1, &m_colourBuffer, Render::VertexDataType::Float, 3);
	m_vertexArray.Create();

	return true;
}

bool RenderSystem::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	m_window = new Render::Window(Render::Window::Properties("Skeleton Application", 640, 480));
	m_device = new Render::Device(*m_window);
	m_window->Show();

	if (!LoadShaders())
	{
		return false;
	}

	if (!CreateMesh())
	{
		return false;
	}

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
	m_device->BindShaderProgram(m_shaderProgram);
	m_device->DrawArray(m_vertexArray, Render::PrimitiveType::Triangles, 0, 3);
	m_device->Present();
	return !m_quit;
}

void RenderSystem::Shutdown()
{	
	m_shaderProgram.Destroy();
	m_vertexShader.Destroy();
	m_fragmentShader.Destroy();
	m_vertexArray.Destroy();
	m_posBuffer.Destroy();
	m_colourBuffer.Destroy();
	m_window->Hide();
	delete m_device;
	delete m_window;
}