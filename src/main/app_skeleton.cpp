#include "app_skeleton.h"
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
#include <algorithm>

enum class Materials : uint8_t
{
	Air,
	Walls, 
	Floor,
	Carpet,
	Pillars,
	OuterWall
};

struct RaymarchTester
{
	uint8_t m_value;
	SDE::DebugRender* m_dbgRender;
	bool operator()(const Vox::ModelRaymarcherParams<VoxelModel>& params)
	{
		if (params.VoxelData() != 0)
		{
			m_dbgRender->AddAxisAtPoint(params.VoxelPosition());
		}
		return true;	// Keep going
	}
};

uint8_t Sphere(const glm::vec3& voxelPosition, const glm::vec3& center, float radius, uint8_t value)
{
	if (glm::distance(voxelPosition, center) <= radius)
	{
		return value;
	}
	else
	{
		return static_cast<uint8_t>(Materials::Air);
	}
}

inline float Box(const glm::vec3& voxelPosition, const glm::vec3& boxMin, const glm::vec3& boxMax)
{
	const glm::vec3 bpos = (boxMax + boxMin) * 0.5f;
	const glm::vec3 b = (boxMax - boxMin) * 0.5f;
	return glm::length(glm::max(glm::abs(bpos - voxelPosition) - b, glm::vec3(0.0)));
}

inline bool Pillar(Vox::ModelAreaDataWriterParams<VoxelModel>& areaParams, int x, int y, int z, const glm::vec3& vox, const glm::vec3& center)
{
	float pillar = Box(vox, center - glm::vec3(0.5f, 0.0f, 0.5f), center + glm::vec3(0.5f, 0.5f, 0.5f));
	pillar = std::min(pillar, Box(vox, center - glm::vec3(0.25f, -0.5f, 0.25f), center + glm::vec3(0.25f, 7.5f, 0.25f)));
	pillar = std::min(pillar, Box(vox, center - glm::vec3(0.5f, -7.5f, 0.5f), center + glm::vec3(0.5f, 8.0f, 0.5f)));
	if (pillar <= 0.0f)
	{
		if (Box(vox, center - glm::vec3(0.125f, 0.0f, 0.125f), center + glm::vec3(0.125f, 8.0f, 0.125f)) <= 0.0f)
		{
			areaParams.WriteVoxel(x, y, z, static_cast<uint8_t>(Materials::OuterWall));
			return true;
		}
		else
		{
			areaParams.WriteVoxel(x, y, z, static_cast<uint8_t>(Materials::Pillars));
			return true;
		}
	}
	return false;
}

struct SphereFiller
{
	glm::vec3 m_center;
	float m_radius;
	uint8_t m_value;
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
						areaParams.WriteVoxel(vx, vy, vz, m_value);
					}
				}
			}
		}
	}
};

struct TestRoomBuilder
{
	void operator()(Vox::ModelAreaDataWriterParams<VoxelModel>& areaParams)
	{
		for (int32_t vz = areaParams.StartVoxel().z; vz != areaParams.EndVoxel().z; ++vz)
		{
			for (int32_t vy = areaParams.StartVoxel().y; vy != areaParams.EndVoxel().y; ++vy)
			{
				for (int32_t vx = areaParams.StartVoxel().x; vx != areaParams.EndVoxel().x; ++vx)
				{
					glm::vec3 vPos = areaParams.VoxelPosition(vx, vy, vz);
					uint8_t val = static_cast<uint8_t>(Materials::Air);

					// first apply outer walls
					float outerWall = Box(vPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 8.0f, 128.0f));
					outerWall = std::min(outerWall, Box(vPos, glm::vec3(127.5f, 0.0f, 0.0f), glm::vec3(128.0f, 8.0f, 128.0f)));
					outerWall = std::min(outerWall, Box(vPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(128.0f, 8.0f, 0.5f)));
					outerWall = std::min(outerWall, Box(vPos, glm::vec3(0.0f, 0.0f, 127.5f), glm::vec3(128.0f, 8.0f, 128.0f)));
					outerWall = std::min(outerWall, Box(vPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(128.0f, 0.125f, 128.0f)));
					if (outerWall <= 0.0f)
					{
						areaParams.WriteVoxel(vx, vy, vz, static_cast<uint8_t>(Materials::OuterWall));
						continue;
					}

					glm::vec3 wallSpacePos = glm::mod(vPos, glm::vec3(16.0f, 8.0f, 16.0f));

					// apply carpets to each inner room
					if (Box(wallSpacePos, glm::vec3(2.0f, 0.125f, 2.0f), glm::vec3(14.0f, 0.25f, 14.0f)) <= 0.0f)
					{
						areaParams.WriteVoxel(vx, vy, vz, static_cast<uint8_t>(Materials::Carpet));
						continue;
					}

					// apply floor if no carpet
					if (Box(wallSpacePos, glm::vec3(0.0f, 0.125f, 0.0f), glm::vec3(16.0f, 0.25f, 16.0f)) <= 0.0f)
					{
						areaParams.WriteVoxel(vx, vy, vz, static_cast<uint8_t>(Materials::Floor));
						continue;
					}

					float innerWall = Box(wallSpacePos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 8.0f, 16.0f));
					innerWall = std::min(innerWall,Box(wallSpacePos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(16.0f, 8.0f, 0.5f)));
					if (innerWall <= 0.0f)
					{
						float door = Box(wallSpacePos, glm::vec3(6.0f, 0.25f, 0.0f), glm::vec3(10.0f, 6.0f, 0.5f));
						door = std::min(door,Box(wallSpacePos, glm::vec3(0.0f, 0.125f, 6.0f), glm::vec3(0.5f, 6.0f, 10.0f)));
						if (door > 0.0f)
						{
							float innerSteelWall = Box(wallSpacePos, glm::vec3(0.0f + 0.125f, 0.0f, 0.0f), glm::vec3(0.25f + 0.125f, 8.0f, 16.0f));
							innerSteelWall = std::min(innerSteelWall, Box(wallSpacePos, glm::vec3(0.0f, 0.0f, 0.0f + 0.125f), glm::vec3(16.0f, 8.0f, 0.5f - 0.125f)));

							areaParams.WriteVoxel(vx, vy, vz, innerSteelWall <= 0.0f ? static_cast<uint8_t>(Materials::OuterWall) : static_cast<uint8_t>(Materials::Walls) );
						}
						continue;
					}

					Pillar(areaParams, vx, vy, vz, wallSpacePos, glm::vec3(3.0f, 0.0f, 3.0f));
					Pillar(areaParams, vx, vy, vz, wallSpacePos, glm::vec3(13.0f, 0.0f, 3.0f));
					Pillar(areaParams, vx, vy, vz, wallSpacePos, glm::vec3(3.0f, 0.0f, 13.0f));
					Pillar(areaParams, vx, vy, vz, wallSpacePos, glm::vec3(13.0f, 0.0f, 13.0f));
				}
			}
		}
	}
};

void AppSkeleton::InitialiseFloor(std::shared_ptr<Assets::Asset>& materialAsset)
{
	// Setup material
	VoxelMaterialSet floorMaterials;
	VoxelMaterial floor, walls, carpet, pillars, outerWall;
	floor.Colour() = glm::vec4(0.165f, 0.498f, 0.251f, 1.0f);
	walls.Colour() = glm::vec4(0.667f, 0.518f, 0.224f, 1.0f);
	carpet.Colour() = glm::vec4(0.18f, 0.263f, 0.447f, 1.0f);
	pillars.Colour() = glm::vec4(0.667f, 0.275f, 0.224f, 1.0f);
	outerWall.Colour() = glm::vec4(0.4f, 0.4f, 0.5f, 1.0f);
	floorMaterials.SetMaterial(static_cast<uint8_t>(Materials::Walls), walls);
	floorMaterials.SetMaterial(static_cast<uint8_t>(Materials::Floor), floor);
	floorMaterials.SetMaterial(static_cast<uint8_t>(Materials::Carpet), carpet);
	floorMaterials.SetMaterial(static_cast<uint8_t>(Materials::Pillars), pillars);
	floorMaterials.SetMaterial(static_cast<uint8_t>(Materials::OuterWall), outerWall);
	floorMaterials.SetRenderMaterialAsset(materialAsset);

	m_testFloor = std::make_unique<Floor>();
	m_testFloor->Create(m_jobSystem, floorMaterials, glm::vec3(128.0f, 8.0f, 128.0f), 16);

	TestRoomBuilder valFiller;
	m_testFloor->ModifyData(Math::Box3(glm::vec3(0.0f), glm::vec3(128.0f,8.0f,128.0f)), valFiller, "room");
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

#define RandFloat(max)	max * ((float)rand() / (float)RAND_MAX)

bool AppSkeleton::Tick()
{
	glm::vec3 ddaRayStart = glm::vec3(2.0f, 2.0f, 2.0f);
	glm::vec3 ddaRayEnd = glm::vec3(128.0f, 8.0f, 128.0f);

	RaymarchTester filler;
	filler.m_value = static_cast<uint8_t>(Materials::Pillars);
	filler.m_dbgRender = &m_renderSystem->GetDebugRender();

	Vox::ModelRaymarcher<VoxelModel> rayMarcher(m_testFloor->GetModel());
	rayMarcher.Raymarch(ddaRayStart, ddaRayEnd, filler);

	if (m_testFloor != nullptr)
	{
		m_testFloor->RebuildDirtyMeshes();
#ifndef SDE_DEBUG
		SphereFiller sphere;
		sphere.m_value = 0;
		const int c_numHoles = 8;
		for (int i = 0;i < c_numHoles;++i)
		{
			sphere.m_radius = RandFloat(1.5f);
			sphere.m_center = glm::vec3(RandFloat(128.0f), RandFloat(8.0f), RandFloat(128.0f));
			m_testFloor->ModifyData(Math::Box3(sphere.m_center - sphere.m_radius, sphere.m_center + sphere.m_radius), sphere, "sphere");
		}
#endif
	}

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

	m_renderSystem->GetDebugRender().AddAxisAtPoint(glm::vec3(0.0f));
	m_renderSystem->GetDebugRender().AddAxisAtPoint(glm::vec3(64.0f));
	m_renderSystem->GetDebugRender().AddAxisAtPoint(glm::vec3(128.0f));

	return true;
}

void AppSkeleton::Shutdown()
{	
	m_testFloor = nullptr;
	m_debugCameraController = nullptr;
}