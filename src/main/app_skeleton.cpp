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

inline glm::vec3 VoxelPosition(glm::vec3 clumpOrigin, glm::vec3 voxelSize, glm::vec3 voxelCenter, int vx, int vy, int vz)
{
	return (clumpOrigin + voxelSize * glm::vec3(vx, vy, vz)) + voxelCenter;
}

struct SphereFiller
{
	glm::vec3 m_center;
	float m_radius;
	uint8_t m_value;
	void operator()(VoxelModel::ClumpDataAccessor& accessor, glm::vec3 clumpOrigin, glm::vec3 voxelSize, glm::vec3 voxelCenter)
	{
		for (int z = 0;z < 2;++z)
		{
			for (int y = 0;y < 2;++y)
			{
				for (int x = 0;x < 2;++x)
				{
					const glm::vec3 vPos = VoxelPosition(clumpOrigin, voxelSize, voxelCenter, x, y, z);
					if (glm::distance(vPos, m_center) <= m_radius)
					{
						auto theClump = accessor.GetClump();
						auto& thisVoxel = theClump->VoxelAt(x, y, z);
						if (thisVoxel != static_cast<uint8_t>(Materials::OuterWall))
						{
							thisVoxel = m_value;
						}
					}
				}
			}
		}
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

struct ValueFiller
{
	uint8_t m_value;
	void operator()(VoxelModel::ClumpDataAccessor& accessor, glm::vec3 clumpOrigin, glm::vec3 voxelSize, glm::vec3 voxelCenter)
	{
		auto theClump = accessor.GetClump();
		memset(theClump, m_value, sizeof(VoxelModel::BlockType::ClumpType));
	}
};

inline bool Pillar(VoxelModel::ClumpDataAccessor& accessor, int x, int y, int z, const glm::vec3& vox, const glm::vec3& center)
{
	float pillar = Box(vox, center - glm::vec3(0.5f, 0.0f, 0.5f), center + glm::vec3(0.5f, 0.5f, 0.5f));
	pillar = std::min(pillar, Box(vox, center - glm::vec3(0.25f, -0.5f, 0.25f), center + glm::vec3(0.25f, 7.5f, 0.25f)));
	pillar = std::min(pillar, Box(vox, center - glm::vec3(0.5f, -7.5f, 0.5f), center + glm::vec3(0.5f, 8.0f, 0.5f)));
	if (pillar <= 0.0f)
	{
		if (Box(vox, center - glm::vec3(0.125f, 0.0f, 0.125f), center + glm::vec3(0.125f, 8.0f, 0.125f)) <= 0.0f)
		{
			auto theClump = accessor.GetClump();
			theClump->VoxelAt(x, y, z) = static_cast<uint8_t>(Materials::OuterWall);
			return true;
		}
		else
		{
			auto theClump = accessor.GetClump();
			theClump->VoxelAt(x, y, z) = static_cast<uint8_t>(Materials::Pillars);
			return true;
		}
	}
	return false;
}

struct TestRoomBuilder
{
	void operator()(VoxelModel::ClumpDataAccessor& accessor, glm::vec3 clumpOrigin, glm::vec3 voxelSize, glm::vec3 voxelCenter)
	{
		for (int z = 0;z < 2;++z)
		{
			for (int y = 0;y < 2;++y)
			{
				for (int x = 0;x < 2;++x)
				{
					glm::vec3 vPos = VoxelPosition(clumpOrigin, voxelSize, voxelCenter, x, y, z);
					uint8_t val = static_cast<uint8_t>(Materials::Air);

					// first apply outer walls
					float outerWall = Box(vPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 8.0f, 128.0f));
					outerWall = std::min(outerWall, Box(vPos, glm::vec3(127.5f, 0.0f, 0.0f), glm::vec3(128.0f, 8.0f, 128.0f)));
					outerWall = std::min(outerWall, Box(vPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(128.0f, 8.0f, 0.5f)));
					outerWall = std::min(outerWall, Box(vPos, glm::vec3(0.0f, 0.0f, 127.5f), glm::vec3(128.0f, 8.0f, 128.0f)));
					outerWall = std::min(outerWall, Box(vPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(128.0f, 0.125f, 128.0f)));
					if (outerWall <= 0.0f)
					{
						auto theClump = accessor.GetClump();
						theClump->VoxelAt(x, y, z) = static_cast<uint8_t>(Materials::OuterWall);
						continue;
					}

					glm::vec3 wallSpacePos = glm::mod(vPos, glm::vec3(16.0f, 8.0f, 16.0f));

					// apply carpets to each inner room
					if (Box(wallSpacePos, glm::vec3(2.0f, 0.125f, 2.0f), glm::vec3(14.0f, 0.25f, 14.0f)) <= 0.0f)
					{
						auto theClump = accessor.GetClump();
						theClump->VoxelAt(x, y, z) = static_cast<uint8_t>(Materials::Carpet);
						continue;
					}

					// apply floor if no carpet
					if (Box(wallSpacePos, glm::vec3(0.0f, 0.125f, 0.0f), glm::vec3(16.0f, 0.25f, 16.0f)) <= 0.0f)
					{
						auto theClump = accessor.GetClump();
						theClump->VoxelAt(x, y, z) = static_cast<uint8_t>(Materials::Floor);
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

							auto theClump = accessor.GetClump();
							theClump->VoxelAt(x, y, z) = innerSteelWall <= 0.0f ? static_cast<uint8_t>(Materials::OuterWall) : static_cast<uint8_t>(Materials::Walls);
						}
						continue;
					}

					Pillar(accessor, x, y, z, wallSpacePos, glm::vec3(8.0f, 0.0f, 8.0f));
				}
			}
		}
	}
};

void AddPopJob(SDE::JobSystem* js, Floor* f, const Math::Box3& target)
{
	auto jobFn = [f, target]()
	{
		SDE_LOG("Running %f, %f, %f", target.Min().x, target.Min().y, target.Min().z);
		double elapsed = 0.0f;
		{
			Core::ScopedTimer time(elapsed);
			TestRoomBuilder valFiller;
			f->ModifyData(target, valFiller);
		}
		SDE_LOG("%f, %f, %f - %f, %f", target.Min().x, target.Min().y, target.Min().z, (float)elapsed, (float)elapsed * 1000.0f);
	};
	js->PushJob(jobFn);
}

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
	m_testFloor->Create(floorMaterials, glm::vec3(128.0f, 8.0f, 128.0f), 16);

	const int32_t c_jobsPerAxis = 16;
	const Math::Box3 c_populationBounds(glm::vec3(0.0f), glm::vec3(128.0f));
	glm::vec3 areaPerJob = c_populationBounds.Size();
	areaPerJob.x = areaPerJob.x / (float)c_jobsPerAxis;
	areaPerJob.z = areaPerJob.z / (float)c_jobsPerAxis;
	glm::vec3 startPos = c_populationBounds.Min();
	for (int32_t x = 0; x < c_jobsPerAxis; ++x)
	{
		for (int32_t y = 0; y < c_jobsPerAxis; ++y)
		{
			AddPopJob(m_jobSystem, m_testFloor.get(), Math::Box3(startPos, startPos + areaPerJob));
			startPos.x = x * areaPerJob.x;
			startPos.z = y * areaPerJob.z;
		}
	}
	
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
	if (m_testFloor != nullptr)
	{
//#ifndef SDE_DEBUG
//		SphereFiller sphere;
//		sphere.m_value = 0;
//		const int c_numHoles = 8;
//		for (int i = 0;i < c_numHoles;++i)
//		{
//			sphere.m_radius = RandFloat(1.5f);
//			sphere.m_center = glm::vec3(RandFloat(128.0f), RandFloat(8.0f), RandFloat(128.0f));
//			m_testFloor->ModifyData(Math::Box3(sphere.m_center - sphere.m_radius, sphere.m_center + sphere.m_radius), sphere);
//		}
//#endif
		m_testFloor->RebuildDirtyMeshes();
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

	// raycast into floor using camera
	glm::vec3 rayStart = forwardPass.GetCamera().Position();
	glm::vec3 rayEnd = forwardPass.GetCamera().Target();
	rayEnd = rayStart + (glm::normalize(rayEnd - rayStart) * 128.0f);
	float tIntersect = 0.0f;
	if (m_testFloor != nullptr && m_testFloor->RayIntersectsRoom(rayStart, rayEnd, tIntersect))
	{
		glm::vec3 intersectPoint = rayStart + ((rayEnd - rayStart) * tIntersect);
		m_renderSystem->GetDebugRender().AddAxisAtPoint(intersectPoint);
	}

	m_renderSystem->GetDebugRender().AddAxisAtPoint(glm::vec3(0.0f));
	m_renderSystem->GetDebugRender().AddAxisAtPoint(glm::vec3(4.0f));

	return true;
}

void AppSkeleton::Shutdown()
{	
	m_testFloor = nullptr;
	m_debugCameraController = nullptr;
}