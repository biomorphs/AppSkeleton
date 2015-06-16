#include "render_system.h"
#include "render/window.h"
#include "render/device.h"
#include "render/mesh_builder.h"
#include "kernel/assert.h"
#include "vox/block.h"
#include "vox/paged_blocks.h"

RenderSystem::RenderSystem()
: m_quit(false)
, m_window(nullptr)
, m_device(nullptr)
{
}

RenderSystem::~RenderSystem()
{
}

bool RenderSystem::CreateMaterial()
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

	m_material.SetShaderProgram(&m_shaderProgram);

	return true;
}

bool RenderSystem::CreateMesh()
{
	Render::MeshBuilder meshBuilder;
	uint32_t posStream = meshBuilder.AddVertexStream(3);
	uint32_t colourStream = meshBuilder.AddVertexStream(3);
	
	meshBuilder.BeginChunk();
	meshBuilder.AddTriangle(posStream, glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(0.5f, -0.5f, 0.0f));
	meshBuilder.AddTriangle(colourStream, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	meshBuilder.EndChunk();

	meshBuilder.CreateMesh(m_mesh);
	m_mesh.SetMaterial(&m_material);

	return true;
}

bool RenderSystem::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	m_window = new Render::Window(Render::Window::Properties("Skeleton Application", 640, 480));
	m_device = new Render::Device(*m_window);
	m_window->Show();

	CreateMaterial();
	CreateMesh();

	Vox::Block<uint8_t, 16> testBlock;
	Vox::Block<uint8_t, 16> other(std::move(testBlock));
	Vox::Block<uint8_t, 16> another;
	another = std::move(other);
	another = std::move(another);

	auto& clumpX2Y0Z1 = another.ClumpAt(2, 0, 1);
	clumpX2Y0Z1.VoxelAt(1, 0, 1) = 5;
	SDE_ASSERT(another.VoxelAt(5, 0, 3) == clumpX2Y0Z1.VoxelAt(1, 0, 1));

	const Vox::Block<uint8_t, 16> constBlock;
	const auto& constClumpX2Y0Z1 = constBlock.ClumpAt(2, 0, 1);
	SDE_ASSERT(&constBlock.VoxelAt(5, 0, 3) == &constClumpX2Y0Z1.VoxelAt(1, 0, 1));

	Vox::PagedBlocks< Vox::Block<uint8_t, 16 > > pagedBlocks;
	auto block = pagedBlocks.BlockAt(10, 2, 12);
	auto blockSearch = pagedBlocks.BlockAt(10, 2, 12);
	SDE_ASSERT(block == blockSearch);

	// find clump 163, 37, 199 (should be equal to block 10,2,12, chunk 3,5,7
	auto& testClump = block->ClumpAt(3, 5, 7);
	auto testClump2 = pagedBlocks.ClumpAt(163, 37, 199);
	SDE_ASSERT(&testClump == testClump2);

	// find voxel 327, 74, 399, which should be = voxel 1,0,1 in clump 163, 37, 199
	auto testVoxel = pagedBlocks.VoxelAt(327, 74, 399);
	auto testVoxel2 = &testClump2->VoxelAt(1, 0, 1);
	SDE_ASSERT(testVoxel == testVoxel2);

	pagedBlocks.VoxelAt(56252, 1824, 10257);

	SDE_LOG("%d pages using %d bytes", (int)pagedBlocks.TotalBlocks(), (int)pagedBlocks.TotalVoxelMemory());

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
	m_device->BindShaderProgram(*m_mesh.GetMaterial()->GetShaderProgram());
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