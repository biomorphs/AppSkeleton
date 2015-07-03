#include "app_skeleton.h"
#include "core/system_enumerator.h"
#include "engine/input_system.h"
#include "render/mesh_builder.h"
#include "render/material_asset.h"
#include "render/shader_program_asset.h"
#include "sde/debug_camera_controller.h"
#include "sde/render_system.h"
#include "sde/asset_system.h"
#include "sde/debug_render.h"

#include "vox/model.h"
#include "vox/quad_extractor.h"

struct VoxelAllocator
{
	static void* AllocateBlock(size_t size)
	{
		void* ptr = malloc(size);
		memset(ptr, 0, size);
		return ptr;
	}
	static void FreeBlock(void* block)
	{
		free(block);
	}
};

typedef uint8_t VoxelType;
typedef Vox::Model<VoxelType, 32, VoxelAllocator> ModelType;
static ModelType s_testModel;

AppSkeleton::AppSkeleton()
: m_quit(false)
{
}

AppSkeleton::~AppSkeleton()
{
}

bool AppSkeleton::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	m_inputSystem = (Engine::InputSystem*)systemEnumerator.GetSystem("Input");
	m_renderSystem = (SDE::RenderSystem*)systemEnumerator.GetSystem("Render");
	m_assetSystem = (SDE::AssetSystem*)systemEnumerator.GetSystem("Assets");
	return true;
}

inline glm::vec3 VoxelPosition(glm::vec3 clumpOrigin, glm::vec3 voxelSize, glm::vec3 voxelCenter, int vx, int vy, int vz)
{
	return (clumpOrigin + voxelSize * glm::vec3(vx, vy, vz)) + voxelCenter;
}

void AppSkeleton::InitialiseVoxelModel()
{
	struct SphereFiller
	{
		void operator()(ModelType::ClumpDataAccessor& accessor, glm::vec3 clumpOrigin, glm::vec3 voxelSize, glm::vec3 voxelCenter)
		{
			const glm::vec3 spherePos(0.0f, 0.0f, 0.0f);
			const float c_radius = 2.0f;

			for (int z = 0;z < 2;++z)
			{
				for (int y = 0;y < 2;++y)
				{
					for (int x = 0;x < 2;++x)
					{
						const glm::vec3 vPos = VoxelPosition(clumpOrigin, voxelSize, voxelCenter, x, y, z);
						if (glm::distance(vPos, spherePos) <= c_radius)
						{
							auto theClump = accessor.GetClump();
							theClump->VoxelAt(x, y, z) = 1;
						}
					}
				}
			}
		}
	};	

	struct CubeFiller
	{
		void operator()(ModelType::ClumpDataAccessor& accessor, glm::vec3 clumpOrigin, glm::vec3 voxelSize, glm::vec3 voxelCenter)
		{
			const glm::vec3 spherePos(0.0f, 0.0f, 0.0f);
			const float c_radius = 2.0f;

			auto theClump = accessor.GetClump();
			memset(theClump, 1, sizeof(ModelType::BlockType::ClumpType));
		}
	};

	s_testModel.SetVoxelSize(glm::vec3(0.25f));

	SphereFiller filler;
	s_testModel.IterateForArea(Math::Box3(glm::vec3(-8.0f), glm::vec3(8.0f)), ModelType::IteratorAccess::ReadWrite, filler);

	CubeFiller cubeFiller;
	s_testModel.IterateForArea(Math::Box3(glm::vec3(-8.0f, -4.0f, -8.0f), glm::vec3(8.0f, 4.0f, -7.5f)), ModelType::IteratorAccess::ReadWrite, cubeFiller);
	s_testModel.IterateForArea(Math::Box3(glm::vec3(-8.0f, -4.0f, 7.5f), glm::vec3(8.0f, 4.0f, 8.0f)), ModelType::IteratorAccess::ReadWrite, cubeFiller);
	s_testModel.IterateForArea(Math::Box3(glm::vec3(-8.0f, -4.0f, -8.0f), glm::vec3(-7.5f, 4.0f, 8.0f)), ModelType::IteratorAccess::ReadWrite, cubeFiller);
	s_testModel.IterateForArea(Math::Box3(glm::vec3(7.5f, -4.0f, -8.0f), glm::vec3(8.0f, 4.0f, 8.0f)), ModelType::IteratorAccess::ReadWrite, cubeFiller);
	s_testModel.IterateForArea(Math::Box3(glm::vec3(-8.0f, -4.0f, -8.0f), glm::vec3(8.0f, -3.5f, 8.0f)), ModelType::IteratorAccess::ReadWrite, cubeFiller);
}

void AppSkeleton::DebugRenderVoxelModel()
{
	const glm::vec4 c_zAxisColour(0.0f, 0.0f, 1.0f, 1.0f);
	const glm::vec4 c_yAxisColour(0.0f, 1.0f, 0.0f, 1.0f);
	const glm::vec4 c_xAxisColour(1.0f, 0.0f, 0.0f, 1.0f);

	Vox::QuadExtractor<ModelType> extractor(s_testModel);
	extractor.ExtractQuads(Math::Box3(glm::vec3(-16.0f), glm::vec3(16.0f)));
	for (auto q = extractor.BeginZAxis(); q != extractor.EndZAxis(); ++q)
	{
		m_renderSystem->GetDebugRender().AddLine(q->m_vertices[0], q->m_vertices[1], c_zAxisColour, c_zAxisColour);
		m_renderSystem->GetDebugRender().AddLine(q->m_vertices[1], q->m_vertices[2], c_zAxisColour, c_zAxisColour);
		m_renderSystem->GetDebugRender().AddLine(q->m_vertices[2], q->m_vertices[3], c_zAxisColour, c_zAxisColour);
		m_renderSystem->GetDebugRender().AddLine(q->m_vertices[3], q->m_vertices[0], c_zAxisColour, c_zAxisColour);
	}

	for (auto q = extractor.BeginYAxis(); q != extractor.EndYAxis(); ++q)
	{
		m_renderSystem->GetDebugRender().AddLine(q->m_vertices[0], q->m_vertices[1], c_yAxisColour, c_yAxisColour);
		m_renderSystem->GetDebugRender().AddLine(q->m_vertices[1], q->m_vertices[2], c_yAxisColour, c_yAxisColour);
		m_renderSystem->GetDebugRender().AddLine(q->m_vertices[2], q->m_vertices[3], c_yAxisColour, c_yAxisColour);
		m_renderSystem->GetDebugRender().AddLine(q->m_vertices[3], q->m_vertices[0], c_yAxisColour, c_yAxisColour);
	}

	for (auto q = extractor.BeginXAxis(); q != extractor.EndXAxis(); ++q)
	{
		m_renderSystem->GetDebugRender().AddLine(q->m_vertices[0], q->m_vertices[1], c_xAxisColour, c_xAxisColour);
		m_renderSystem->GetDebugRender().AddLine(q->m_vertices[1], q->m_vertices[2], c_xAxisColour, c_xAxisColour);
		m_renderSystem->GetDebugRender().AddLine(q->m_vertices[2], q->m_vertices[3], c_xAxisColour, c_xAxisColour);
		m_renderSystem->GetDebugRender().AddLine(q->m_vertices[3], q->m_vertices[0], c_xAxisColour, c_xAxisColour);
	}
}

bool AppSkeleton::PostInit()
{
	// register asset factories
	auto& assetCreator = m_assetSystem->GetCreator();
	assetCreator.RegisterFactory<Render::MaterialAssetFactory>(Render::MaterialAsset::c_assetType);
	assetCreator.RegisterFactory<Render::ShaderProgramAssetFactory>(Render::ShaderProgramAsset::c_assetType);

	// Set up camera controller and render passes
	m_debugCameraController = std::make_unique<SDE::DebugCameraController>();
	m_forwardPassId = m_renderSystem->CreatePass("Forward");

	// load material, on completion we build the mesh
	m_assetSystem->LoadAsset("simple_material", [this](const std::string& asset, bool result)
	{
		auto loadedAsset = this->m_assetSystem->GetAsset(asset);
		auto renderMaterial = static_cast<Render::MaterialAsset*>(loadedAsset.get());
		this->CreateMesh();
		this->m_mesh->SetMaterial(renderMaterial->GetMaterial());
	});

	InitialiseVoxelModel();

	return true;
}

bool AppSkeleton::CreateMesh()
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

	m_mesh = std::make_unique<Render::Mesh>();
	meshBuilder.CreateMesh(*m_mesh);

	return true;
}

bool AppSkeleton::Tick()
{
	auto& forwardPass = m_renderSystem->GetPass(m_forwardPassId);

	// apply camera controller to passes
	m_debugCameraController->Update(*m_inputSystem->ControllerState(0), 0.016);
	m_debugCameraController->ApplyToCamera(forwardPass.GetCamera());
	m_debugCameraController->ApplyToCamera(m_renderSystem->DebugCamera());

	// Rendering submission
	if (m_mesh != nullptr)
	{
		//forwardPass.AddInstance(m_mesh.get(), glm::mat4());
	}

	DebugRenderVoxelModel();

	return !m_quit;
}

void AppSkeleton::OnEventRecieved(const Core::EngineEvent& e)
{
	if (e.m_type == Core::EngineEvent::QuitRequest)
	{
		m_quit = true;
	}
}

void AppSkeleton::Shutdown()
{	
	m_debugCameraController = nullptr;
	m_mesh = nullptr;
}