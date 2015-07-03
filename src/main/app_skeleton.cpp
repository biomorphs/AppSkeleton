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
		forwardPass.AddInstance(m_mesh.get(), glm::mat4());
	}
	m_renderSystem->GetDebugRender().AddAxisAtPoint(glm::vec3(0.0f), 1.0f);

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