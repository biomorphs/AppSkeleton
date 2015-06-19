#include "render_system.h"
#include "kernel/assert.h"
#include "render/window.h"
#include "render/device.h"
#include "render/mesh_builder.h"
#include "render/shader_binary.h"
#include "render/shader_program.h"
#include "render/material.h"
#include "vox/block.h"
#include "vox/paged_blocks.h"
#include "vox/model.h"

RenderSystem::RenderSystem()
: m_quit(false)
, m_window(nullptr)
, m_device(nullptr)
{
}

RenderSystem::~RenderSystem()
{
}

std::shared_ptr<Render::Material> RenderSystem::CreateMaterial()
{
	std::string compileResult;
	Render::ShaderBinary vertexShader, fragmentShader;
	if (!vertexShader.CompileFromFile(Render::ShaderType::VertexShader, "simple_vertex.txt", compileResult))
	{
		SDE_LOG("Failed to compile shader:\r\n\t%s", compileResult.c_str());
		return false;
	}

	if (!fragmentShader.CompileFromFile(Render::ShaderType::FragmentShader, "simple_fragment.txt", compileResult))
	{
		SDE_LOG("Failed to compile shader:\r\n\t%s", compileResult.c_str());
		return false;
	}

	auto shaderProgram = std::make_shared<Render::ShaderProgram>();
	if (!shaderProgram->Create(vertexShader, fragmentShader, compileResult))
	{
		SDE_LOG("Failed to link shaders: \r\n\t%s", compileResult.c_str());
		return false;
	}

	auto material = std::make_shared<Render::Material>();
	material->SetShaderProgram(shaderProgram);
	material->GlobalDefinitions().m_mvpUniformHandle = shaderProgram->GetUniformHandle("MVP");

	return material;
}

bool RenderSystem::CreateMesh()
{
	Render::MeshBuilder meshBuilder;
	uint32_t posStream = meshBuilder.AddVertexStream(3);
	uint32_t colourStream = meshBuilder.AddVertexStream(3);
	
	meshBuilder.BeginChunk();
	{
		meshBuilder.BeginTriangle();
		{
			meshBuilder.SetStreamData(posStream, 
									  glm::vec3(-0.5f, -0.5f, 0.0f), 
									  glm::vec3(0.5f, -0.5f, 0.0f), 
									  glm::vec3(0.5f, 0.5f, 0.0f));
			meshBuilder.SetStreamData(colourStream, 
									  glm::vec3(1.0f, 0.0f, 0.0f), 
									  glm::vec3(0.0f, 1.0f, 0.0f), 
									  glm::vec3(0.0f, 0.0f, 1.0f));
		}
		meshBuilder.EndTriangle();
		meshBuilder.BeginTriangle();
		{
			meshBuilder.SetStreamData(posStream,
									  glm::vec3(-0.5f, -0.5f, 0.0f),
									  glm::vec3(0.5f, 0.5f, 0.0f),
									  glm::vec3(-0.5f, 0.5f, 0.0f));
			meshBuilder.SetStreamData(colourStream,
									  glm::vec3(1.0f, 0.0f, 0.0f),
									  glm::vec3(0.0f, 0.0f, 1.0f),
									  glm::vec3(0.0f, 1.0f, 0.0f));
		}
		meshBuilder.EndTriangle();
	}
	meshBuilder.EndChunk();

	m_mesh = std::make_shared<Render::Mesh>();
	meshBuilder.CreateMesh(*m_mesh);

	auto material = CreateMaterial();
	m_mesh->SetMaterial(material);

	return true;
}

bool RenderSystem::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	m_window = new Render::Window(Render::Window::Properties("Skeleton Application", 640, 480));
	m_device = new Render::Device(*m_window);
	m_window->Show();

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
	auto block = pagedBlocks.BlockAt(glm::ivec3(10, 2, 12));
	auto blockSearch = pagedBlocks.BlockAt(glm::ivec3(10, 2, 12));
	SDE_ASSERT(block == blockSearch);

	// find clump 163, 37, 199 (should be equal to block 10,2,12, chunk 3,5,7
	auto& testClump = block->ClumpAt(3, 5, 7);
	auto testClump2 = pagedBlocks.ClumpAt(glm::ivec3(163, 37, 199));
	SDE_ASSERT(&testClump == testClump2);

	// find voxel 327, 74, 399, which should be = voxel 1,0,1 in clump 163, 37, 199
	auto testVoxel = pagedBlocks.VoxelAt(glm::ivec3(327, 74, 399));
	auto testVoxel2 = &testClump2->VoxelAt(1, 0, 1);
	SDE_ASSERT(testVoxel == testVoxel2);
	pagedBlocks.VoxelAt(glm::ivec3(56252, 1824, 10257));
	SDE_LOG("%d pages using %d bytes", (int)pagedBlocks.TotalBlocks(), (int)pagedBlocks.TotalVoxelMemory());

	typedef Vox::Model<uint8_t, 32> TestModel;
	TestModel model;
	model.SetVoxelSize(glm::vec3(0.125));

	auto iterFn = [](TestModel::ClumpDataAccessor& theClump, glm::vec3 clumpOrigin, glm::vec3 voxelSize, glm::vec3 voxelCenterOffset)
	{
		// voxel helper to avoid tight loop
		#define VOXELPOS(x,y,z)	clumpOrigin + (glm::vec3(x, y, z) * voxelSize) + voxelCenterOffset
		auto clumpData = theClump.GetClump();
		const glm::vec3 v0Pos = VOXELPOS(0, 0, 0);
		const glm::vec3 v1Pos = VOXELPOS(1, 0, 0);
		const glm::vec3 v2Pos = VOXELPOS(0, 1, 0);
		const glm::vec3 v3Pos = VOXELPOS(1, 1, 0);
		const glm::vec3 v4Pos = VOXELPOS(0, 0, 1);
		const glm::vec3 v5Pos = VOXELPOS(1, 0, 1);
		const glm::vec3 v6Pos = VOXELPOS(0, 1, 1);
		const glm::vec3 v7Pos = VOXELPOS(1, 1, 1);
	};

 	model.IterateForArea(Math::Box3(glm::vec3(0.0f), glm::vec3(128.0f)), TestModel::IteratorAccess::ReadWrite, iterFn);

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
	static float r = 0.0f;
	r += 0.0001f;
	glm::mat4 modelMat = glm::rotate(glm::mat4(), r, glm::vec3(0.0f, 1.0f, 0.0f));
	m_forwardPass.GetCamera().LookAt(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_forwardPass.AddInstance(m_mesh, modelMat);

	m_device->ClearColourDepthTarget(glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
	m_forwardPass.RenderAll(*m_device);
	m_device->Present();
	m_forwardPass.Reset();
	return !m_quit;
}

void RenderSystem::Shutdown()
{	
	m_mesh = nullptr;
	m_window->Hide();
	delete m_device;
	delete m_window;
}