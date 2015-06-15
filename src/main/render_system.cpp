#include "render_system.h"
#include "render/window.h"
#include "render/device.h"
#include "render/mesh_builder.h"
#include "kernel/assert.h"
#include <glm.hpp>

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
	Render::MeshBuilder meshBuilder;
	uint32_t posStream = meshBuilder.AddVertexStream(3);
	uint32_t colourStream = meshBuilder.AddVertexStream(3);
	
	meshBuilder.BeginChunk();
	meshBuilder.AddTriangleData(posStream, glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(-1.0f, -0.5f, 0.0f), glm::vec3(1.0f, -0.5f, 0.0f));
	meshBuilder.AddTriangleData(colourStream, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	meshBuilder.EndChunk();

	meshBuilder.CreateMesh(m_mesh);

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
	m_device->DrawArray(m_mesh.GetVertexArray(), Render::PrimitiveType::Triangles, 0, 3);
	m_device->Present();
	return !m_quit;
}

void RenderSystem::Shutdown()
{	
	m_mesh.Destroy();
	m_shaderProgram.Destroy();
	m_vertexShader.Destroy();
	m_fragmentShader.Destroy();
	m_window->Hide();
	delete m_device;
	delete m_window;
}