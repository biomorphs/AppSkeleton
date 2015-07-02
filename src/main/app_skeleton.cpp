#include "app_skeleton.h"
#include "kernel/assert.h"
#include "core/system_enumerator.h"
#include "engine/input_system.h"
#include "render/mesh_builder.h"
#include "render/shader_binary.h"
#include "render/shader_program.h"
#include "render/material.h"
#include "render/material_asset.h"
#include "render/shader_program_asset.h"
#include "sde/debug_camera_controller.h"
#include "sde/render_system.h"
#include "core/asset_creator.h"
#include "core/asset_serialiser.h"
#include "core/asset_database.h"

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
	return true;
}

bool AppSkeleton::PostInit()
{
	Core::AssetDatabase m_assets;

	Core::AssetCreator assetCreator;
	assetCreator.RegisterFactory<Render::MaterialAssetFactory>(Render::MaterialAsset::c_assetType);
	assetCreator.RegisterFactory<Render::ShaderProgramAssetFactory>(Render::ShaderProgramAsset::c_assetType);

	Core::AssetSerialiser assetLoader(m_assets, assetCreator);
 	if (!assetLoader.Load("assets", "simple_material"))
	{
		return false;
	}

	CreateMesh();
	m_debugCameraController = std::make_unique<SDE::DebugCameraController>();
	m_forwardPassId = m_renderSystem->CreatePass("Forward");
	return true;
}

std::shared_ptr<Render::Material> AppSkeleton::CreateMaterial()
{
	std::string compileResult;
	Render::ShaderBinary vertexShader, fragmentShader;
	if (!vertexShader.CompileFromFile(Render::ShaderType::VertexShader, "shaders/simple_vertex.txt", compileResult))
	{
		SDE_LOG("Failed to compile shader:\r\n\t%s", compileResult.c_str());
		return false;
	}

	if (!fragmentShader.CompileFromFile(Render::ShaderType::FragmentShader, "shaders/simple_fragment.txt", compileResult))
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

	m_mesh = std::make_shared<Render::Mesh>();
	meshBuilder.CreateMesh(*m_mesh);

	auto material = CreateMaterial();
	m_mesh->SetMaterial(material);

	return true;
}

void AppSkeleton::OnEventRecieved(const Core::EngineEvent& e)
{
	if (e.m_type == Core::EngineEvent::QuitRequest)
	{
		m_quit = true;
	}
}

bool AppSkeleton::Tick()
{
	m_debugCameraController->Update(*m_inputSystem->ControllerState(0), 0.016 );
	
	auto& forwardPass = m_renderSystem->GetPass(m_forwardPassId);
	m_debugCameraController->ApplyToCamera(forwardPass.GetCamera());
	forwardPass.AddInstance(m_mesh, glm::mat4());

	return !m_quit;
}

void AppSkeleton::Shutdown()
{	
	m_mesh = nullptr;
}