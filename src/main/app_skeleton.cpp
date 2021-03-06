#include "app_skeleton.h"
#include "test_room_builder.h"
#include "particle_manager.h"
#include "particle_effect.h"
#include "particle_effects.h"
#include "particles_stats.h"

#include "core/system_enumerator.h"
#include "core/timer.h"
#include "input/input_system.h"
#include "debug_gui/debug_gui_system.h"
#include "render/material_asset.h"
#include "render/shader_program_asset.h"
#include "render/texture_asset.h"
#include "sde/debug_camera_controller.h"
#include "sde/render_system.h"
#include "sde/asset_system.h"
#include "sde/debug_render.h"
#include "sde/font_asset.h"
#include "sde/job_system.h"
#include "vox/model_ray_marcher.h"

#include "particle_tests.h"

struct ShotTest
{
	glm::vec3 m_center;
	float m_radius;
	void operator()(Vox::ModelAreaDataWriterParams<VoxelModel>& areaParams)
	{
		for (int32_t vz = areaParams.StartVoxel().z; vz != areaParams.EndVoxel().z; ++vz)
		{
			for (int32_t vy = areaParams.StartVoxel().y; vy != areaParams.EndVoxel().y; ++vy)
			{
				for (int32_t vx = areaParams.StartVoxel().x; vx != areaParams.EndVoxel().x; ++vx)
				{
					const glm::vec3 vPos = areaParams.VoxelPosition(vx, vy, vz);
					if (glm::distance(vPos, m_center) <= m_radius)
					{
						auto& voxel = areaParams.VoxelAt(vx, vy, vz);
						if (voxel != static_cast<uint8_t>(Materials::Air))
						{
							uint8_t voxelDamage = GetVoxelDamage(voxel);
							if (voxelDamage < 3)
							{
								voxelDamage++;
								voxel = PackVoxel(GetVoxelMaterial(voxel), voxelDamage);
							}
							else if(GetVoxelMaterial(voxel) != Materials::OuterWall)
							{
								voxel = 0;
							}
						}
					}
				}
			}
		}
	}
};

struct RaymarchTester
{
	Floor* m_floor;
	float m_radius;
	AppSkeleton* m_app;
	bool operator()(const Vox::ModelRaymarcherParams<VoxelModel>& params)
	{
		if (params.VoxelData() != 0)
		{
			auto mat = GetVoxelMaterial(params.VoxelData());
			glm::vec4 particleColour(1.0f);
			switch (mat)
			{
			case Materials::OuterWall:
				particleColour = glm::vec4(0.686f, 0.686f, 0.686f, 1.0f);
				break;
			case Materials::Walls:
				particleColour = glm::vec4(0.581f, 0.315f, 0.231f, 1.0f);
				break;
			case Materials::Floor:
				particleColour = glm::vec4(0.9f, 0.9f, 0.9f, 1.0f);
				break;
			case Materials::Carpet:
				particleColour = glm::vec4(0.269f, 0.574f, 0.261f, 1.0f);
				break;
			case Materials::Pillars:
				particleColour = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
				break;
			}
			m_app->SpawnParticlesAt(params.VoxelPosition(), particleColour);
			ShotTest shot;
			shot.m_radius = m_radius;
			shot.m_center = params.VoxelPosition();
			m_floor->ModifyData(Math::Box3(shot.m_center - shot.m_radius, shot.m_center + shot.m_radius), shot);
			return false;
		}
		return true;	// Keep going
	}
};

void AppSkeleton::InitialiseFloor(std::shared_ptr<Assets::Asset>& materialAsset)
{
	// Setup material
	VoxelMaterialSet floorMaterials;

	glm::vec4 baseColours[] = {
		glm::vec4(1.0f,1.0f,1.0f,1.0f),	// Walls
		glm::vec4(1.0f,1.0f,1.0f,1.0f),	// Floor
		glm::vec4(1.0f,1.0f,1.0f,1.0f),	// Carpet
		glm::vec4(1.0f,1.0f,1.0f,1.0f),	// Pillars
		glm::vec4(1.0f,1.0f,1.0f,1.0f)	// Outerwall
	};
	const uint8_t c_maxMaterials = (uint8_t)(sizeof(baseColours) / sizeof(baseColours[0]));
	for (uint8_t m = 0; m < c_maxMaterials; ++m)
	{
		for (uint8_t dmg = 0; dmg < 4; ++dmg)
		{
			VoxelMaterial mat;
			float dmgColourMod = 0.5f + (0.5f * ((float)(3 - dmg) / 3.0f));
			mat.Colour() = dmgColourMod * baseColours[m];
			mat.TextureIndex() = (float)m;
			floorMaterials.SetMaterial(PackVoxel(static_cast<Materials>(m + 1), dmg), mat);
		}
	}
	floorMaterials.SetRenderMaterialAsset(materialAsset);

	m_testFloor = std::make_unique<Floor>();
	m_testFloor->Create(m_jobSystem, floorMaterials, glm::vec3(128.0f, 8.0f, 128.0f), 16);

//	// We now populate the world data, then save it
//	TestRoomBuilder valFiller;
//#ifdef SDE_DEBUG
//	m_testFloor->ModifyDataAndSave(Math::Box3(glm::vec3(0.0f), glm::vec3(16.0f,8.0f,16.0f)), valFiller, "models/test.vox");
//#else
//	m_testFloor->ModifyDataAndSave(Math::Box3(glm::vec3(0.0f), glm::vec3(128.0f, 8.0f, 128.0f)), valFiller, "models/test_big.vox");
//#endif

#ifdef SDE_DEBUG
	m_testFloor->LoadFile("models/test.vox");
#else
	m_testFloor->LoadFile("models/test_big.vox");
#endif
}

AppSkeleton::AppSkeleton()
{
}

AppSkeleton::~AppSkeleton()
{
}

bool AppSkeleton::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	m_inputSystem = (Input::InputSystem*)systemEnumerator.GetSystem("Input");
	m_renderSystem = (SDE::RenderSystem*)systemEnumerator.GetSystem("Render");
	m_assetSystem = (SDE::AssetSystem*)systemEnumerator.GetSystem("Assets");
	m_jobSystem = (SDE::JobSystem*)systemEnumerator.GetSystem("Jobs");
	m_debugGui = (DebugGui::DebugGuiSystem*)systemEnumerator.GetSystem("DebugGui");
	m_particles = (ParticleManager*)systemEnumerator.GetSystem("Particles");

	// Pass init params to renderer
	SDE::RenderSystem::InitialisationParams renderParams(c_windowWidth, c_windowHeight, false, "AppSkeleton");
	m_renderSystem->SetInitialiseParams(renderParams);

	// Set up render passes
	m_forwardPassId = m_renderSystem->CreatePass("Forward");
	m_debugRenderPassId = m_renderSystem->CreatePass("Debug");
	m_renderSystem->GetPass(m_debugRenderPassId).GetRenderState().m_backfaceCullEnabled = false;
	m_renderSystem->GetPass(m_debugRenderPassId).GetRenderState().m_blendingEnabled = true;

	uint32_t guiPassId = m_renderSystem->CreatePass("DebugGui");
	m_renderSystem->GetPass(guiPassId).GetRenderState().m_blendingEnabled = true;
	m_renderSystem->GetPass(guiPassId).GetRenderState().m_depthTestEnabled = false;
	m_renderSystem->GetPass(guiPassId).GetRenderState().m_backfaceCullEnabled = false;

	m_particlesPassId = m_renderSystem->CreatePass("Particles");
	m_renderSystem->GetPass(m_particlesPassId).GetRenderState().m_blendingEnabled = true;
	m_renderSystem->GetPass(m_particlesPassId).GetRenderState().m_backfaceCullEnabled = false;

	// Set up debug gui data
	DebugGui::DebugGuiSystem::InitialisationParams guiParams(m_renderSystem, m_inputSystem, guiPassId);
	m_debugGui->SetInitialiseParams(guiParams);

	// register asset factories
	auto& assetCreator = m_assetSystem->GetCreator();
	assetCreator.RegisterFactory<Render::MaterialAssetFactory>(Render::MaterialAsset::c_assetType);
	assetCreator.RegisterFactory<Render::ShaderProgramAssetFactory>(Render::ShaderProgramAsset::c_assetType);
	assetCreator.RegisterFactory<Render::TextureAssetFactory>(Render::TextureAsset::c_assetType);
	assetCreator.RegisterFactory<SDE::FontAssetFactory>(SDE::FontAsset::c_assetType);

	return true;
}

void AppSkeleton::SpawnParticlesAt(glm::vec3 position, glm::vec4 colour)
{
	ParticleEffect* effect = m_particles->AddEffect(16);
	if (effect && m_pointRender.get() != nullptr)
	{
		effect->AddEmitter(std::shared_ptr<ParticleEmitter>(new ParticleEffects::EmitBurst(16)));

		effect->AddGenerator(std::shared_ptr<ParticleGenerator>(new ParticleEffects::GenerateStaticPosition(position)));
		effect->AddGenerator(std::shared_ptr<ParticleGenerator>(new ParticleEffects::GenerateRandomLifetime(0.5f, 1.5f)));

		static glm::vec3 velMin(-2.0f, -1.0f, -2.0f);
		static glm::vec3 velMax(2.0f, 2.0f, 2.0f);
		effect->AddGenerator(std::shared_ptr<ParticleGenerator>(new ParticleEffects::GenerateRandomVelocity(velMin, velMax)));

		auto colourUpdater = new ParticleEffects::ColourFader(
			colour,
			colour * glm::vec4(0.0f, 0.0f, 0.0f, 0.5f),
			0.0f, 1.5f);
		effect->AddUpdater(std::shared_ptr<ParticleUpdater>(colourUpdater));

		effect->AddUpdater(std::shared_ptr<ParticleUpdater>(new ParticleEffects::EulerFloorBouncer(0.25f)));
		effect->AddUpdater(std::shared_ptr<ParticleUpdater>(new ParticleEffects::GravityUpdater(-5.0f)));
		effect->AddUpdater(std::shared_ptr<ParticleUpdater>(new ParticleEffects::KillOnZeroLife()));
		effect->AddRenderer(m_pointRender);

		effect->SetLifetime(std::shared_ptr<ParticleEffectLifetime>(new ParticleEffects::KillOnZeroParticles()));

		m_particles->StartEffect(effect);
	}
}

void AppSkeleton::InitialiseParticles(std::shared_ptr<Assets::Asset>& materialAsset)
{
	m_pointRender = std::make_shared<PointSpriteParticleRenderer>(m_debugRender.get());
	m_pointRender->Create(materialAsset);
}

bool AppSkeleton::PostInit()
{
	m_debugRender = std::make_unique<SDE::DebugRender>();
	m_debugRender->Create();

	m_debugCameraController = std::make_unique<SDE::DebugCameraController>();
	m_debugCameraController->SetPosition(glm::vec3(64.0f, 20.0f, 64.0f));
	m_camera.SetClipPlanes(0.1f, 256.0f);
	m_camera.SetFOVAndAspectRatio(70.0f, (float)c_windowWidth / (float)c_windowHeight);

	// load material, on completion, create the floor
	m_assetSystem->LoadAsset("simple_diffuse_material", [this](const std::string& asset, bool result)
	{
		if (result)
		{
			auto loadedAsset = this->m_assetSystem->GetAsset(asset);
			InitialiseFloor(loadedAsset);
		}
	});

	// load particle material, on completion, setup particles
	m_assetSystem->LoadAsset("pointsprite_simple_material", [this](const std::string& asset, bool result)
	{
		if (result)
		{
			auto loadedAsset = this->m_assetSystem->GetAsset(asset);
			InitialiseParticles(loadedAsset);
		}
	});

	return true;
}

bool AppSkeleton::Tick()
{
	// Update camera
	m_debugCameraController->Update(*m_inputSystem->ControllerState(0), 0.016);
	m_debugCameraController->ApplyToCamera(m_camera);

	// Update world
	if (m_testFloor != nullptr)
	{
		m_testFloor->Update();
		m_testFloor->DisplayDebugGui(*m_debugGui);
	}

	if ((m_inputSystem->ControllerState(0)->m_buttonState & Input::ControllerButtons::RightShoulder)
		|| m_inputSystem->ControllerState(0)->m_buttonState & Input::ControllerButtons::LeftShoulder)
	{
		RaymarchTester filler;
		filler.m_app = this;
		filler.m_floor = m_testFloor.get();

		uint32_t pellets = 0; 
		float jitterMax = 0.0f;
		if (m_inputSystem->ControllerState(0)->m_buttonState & Input::ControllerButtons::RightShoulder)
		{
			pellets = 10 + rand() % 10;
			jitterMax = 0.25;
			filler.m_radius = 0.25f * ((float)rand() / (float)RAND_MAX);
		}
		else
		{
			pellets = 2 + rand() % 8;
			jitterMax = 0.1f;
			filler.m_radius = 0.125f * ((float)rand() / (float)RAND_MAX);
		}

		for (uint32_t p = 0; p < pellets; ++p)
		{
			const glm::vec3 cameraPos = m_camera.Position();
			const glm::vec3 cameraTarget = m_camera.Target();
			const glm::vec3 cameraDir = glm::normalize(cameraTarget - cameraPos);
			glm::vec3 jitter = glm::vec3(jitterMax * ((float)rand() / (float)RAND_MAX),
				jitterMax * ((float)rand() / (float)RAND_MAX),
				jitterMax * ((float)rand() / (float)RAND_MAX));
			jitter -= jitterMax * 0.5f;
			const glm::vec3 rayEndPos = glm::normalize(cameraDir + jitter) * 128.0f;

			Vox::ModelRaymarcher<VoxelModel> rayMarcher(m_testFloor->GetModel());
			rayMarcher.Raymarch(cameraPos, cameraPos + rayEndPos, filler);
		}
	}

	if ((m_inputSystem->ControllerState(0)->m_buttonState & Input::ControllerButtons::Start))
	{
		if (m_testFloor != nullptr)
		{
			m_testFloor->SaveNow("models/modified.vox");
		}
	}

	if ((m_inputSystem->ControllerState(0)->m_buttonState & Input::ControllerButtons::Back))
	{
		if (m_testFloor != nullptr)
		{
			m_testFloor->LoadFile("models/modified.vox");
		}
	}

	if ((m_inputSystem->ControllerState(0)->m_buttonState & Input::ControllerButtons::X))
	{
		auto camLookAt = glm::normalize(m_camera.Target() - m_camera.Position());
		SpawnParticlesAt(m_camera.Position() + (camLookAt * 2.0f), glm::vec4(0.0f,0.0f,1.0f,1.0f));
	}

	// Particles stats
	static ParticlesStats pStats;
	m_particles->PopulateStats(pStats);
	pStats.DisplayDebugGui(*m_debugGui);
		
	// Rendering
	m_renderSystem->SetClearColour(glm::vec4(0.14f, 0.23f, 0.45f, 1.0f));

	auto& forwardPass = m_renderSystem->GetPass(m_forwardPassId);
	if (m_testFloor != nullptr)
	{
		m_testFloor->Render(m_camera, forwardPass);
	}

	auto &particlePass = m_renderSystem->GetPass(m_particlesPassId);
	if (m_pointRender != nullptr)
	{
		m_pointRender->PushToRenderPass(m_camera, particlePass);
	}

	auto& debugPass = m_renderSystem->GetPass(m_debugRenderPassId);
	m_debugRender->PushToRenderPass(m_camera, debugPass);

	return true;
}

void AppSkeleton::Shutdown()
{	
	m_pointRender = nullptr;
	m_debugRender = nullptr;
	m_testFloor = nullptr;
	m_debugCameraController = nullptr;
}