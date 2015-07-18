#include "app_skeleton.h"
#include "test_room_builder.h"

#include "core/system_enumerator.h"
#include "core/timer.h"
#include "input/input_system.h"
#include "render/material_asset.h"
#include "render/shader_program_asset.h"
#include "sde/debug_camera_controller.h"
#include "sde/render_system.h"
#include "sde/asset_system.h"
#include "sde/debug_render.h"
#include "sde/job_system.h"
#include "vox/model_ray_marcher.h"


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
							else if (GetVoxelMaterial(voxel) != Materials::OuterWall)
							{
								voxel = static_cast<uint8_t>(Materials::Air);
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
	SDE::DebugRender* m_dbgRender;
	Floor* m_floor;
	float m_radius;
	bool operator()(const Vox::ModelRaymarcherParams<VoxelModel>& params)
	{
		if (params.VoxelData() != 0)
		{
			ShotTest shot;
			shot.m_radius = m_radius;
			shot.m_center = params.VoxelPosition();
			m_floor->ModifyData(Math::Box3(shot.m_center - shot.m_radius, shot.m_center + shot.m_radius), shot, "sphere");
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
		glm::vec4(0.667f, 0.518f, 0.224f, 1.0f),	// Walls
		glm::vec4(0.165f, 0.498f, 0.251f, 1.0f),	// Floor
		glm::vec4(0.18f, 0.263f, 0.447f, 1.0f),		// Carpet
		glm::vec4(0.667f, 0.275f, 0.224f, 1.0f),	// Pillars
		glm::vec4(0.4f, 0.4f, 0.5f, 1.0f)			// Outerwall
	};
	const uint8_t c_maxMaterials = (uint8_t)(sizeof(baseColours) / sizeof(baseColours[0]));
	for (uint8_t m = 0; m < c_maxMaterials; ++m)
	{
		for (uint8_t dmg = 0; dmg < 4; ++dmg)
		{
			VoxelMaterial mat;
			float dmgColourMod = 0.3f + (0.7f * ((float)(3 - dmg) / 3.0f));
			mat.Colour() = dmgColourMod * baseColours[m];
			floorMaterials.SetMaterial(PackVoxel(static_cast<Materials>(m + 1), dmg), mat);
		}
	}
	floorMaterials.SetRenderMaterialAsset(materialAsset);

	m_testFloor = std::make_unique<Floor>();
	m_testFloor->Create(m_jobSystem, floorMaterials, glm::vec3(128.0f, 8.0f, 128.0f), 16);

	TestRoomBuilder valFiller;
#ifdef SDE_DEBUG
	m_testFloor->ModifyData(Math::Box3(glm::vec3(0.0f), glm::vec3(8.0f,8.0f,8.0f)), valFiller, "room");
#else
	m_testFloor->ModifyData(Math::Box3(glm::vec3(0.0f), glm::vec3(128.0f, 8.0f, 128.0f)), valFiller, "room");
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
	m_renderSystem->GetPass(m_forwardPassId).GetCamera().SetClipPlanes(0.1f, 256.0f);
	m_renderSystem->GetPass(m_forwardPassId).GetCamera().SetFOVAndAspectRatio(70.0f, 1280.0f / 720.0f);
	m_renderSystem->DebugCamera().SetClipPlanes(0.1f, 256.0f);
	m_renderSystem->DebugCamera().SetFOVAndAspectRatio(70.0f, 1280.0f / 720.0f);
	m_renderSystem->SetClearColour(glm::vec4(0.14f, 0.23f, 0.45f, 1.0f));

	// load material, on completion, create the floor
	m_assetSystem->LoadAsset("simple_diffuse_material", [this](const std::string& asset, bool result)
	{
		if (result)
		{
			auto loadedAsset = this->m_assetSystem->GetAsset(asset);
			InitialiseFloor(loadedAsset);
		}
	});

	return true;
}

bool AppSkeleton::Tick()
{
	if ((m_inputSystem->ControllerState(0)->m_buttonState & Input::ControllerButtons::RightShoulder)
		|| m_inputSystem->ControllerState(0)->m_buttonState & Input::ControllerButtons::LeftShoulder)
	{
		RaymarchTester filler;
		filler.m_dbgRender = &m_renderSystem->GetDebugRender();
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
			const glm::vec3 cameraPos = m_renderSystem->DebugCamera().Position();
			const glm::vec3 cameraTarget = m_renderSystem->DebugCamera().Target();
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

	// Collect floor meshing results
	m_testFloor->RebuildDirtyMeshes();

	// Rendering submission
	auto& forwardPass = m_renderSystem->GetPass(m_forwardPassId);
	if (m_testFloor != nullptr)
	{
		m_testFloor->Render(forwardPass);
	}

	// apply camera controller to passes
	m_debugCameraController->Update(*m_inputSystem->ControllerState(0), 0.016);
	m_debugCameraController->ApplyToCamera(forwardPass.GetCamera());
	m_debugCameraController->ApplyToCamera(m_renderSystem->DebugCamera());

	return true;
}

void AppSkeleton::Shutdown()
{	
	m_testFloor = nullptr;
	m_debugCameraController = nullptr;
}