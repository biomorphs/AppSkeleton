#include "app_skeleton.h"
#include "core/system_enumerator.h"
#include "core/timer.h"
#include "engine/input_system.h"
#include "render/material_asset.h"
#include "render/shader_program_asset.h"
#include "sde/debug_camera_controller.h"
#include "sde/render_system.h"
#include "sde/asset_system.h"
#include "sde/debug_render.h"

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

inline uint8_t Box(const glm::vec3& voxelPosition, const glm::vec3& boxMin, const glm::vec3& boxMax, uint8_t value)
{
	if (glm::all(glm::greaterThanEqual(voxelPosition, boxMin)) && glm::all(glm::lessThanEqual(voxelPosition, boxMax)))
	{
		return value;
	}
	else
	{
		return static_cast<uint8_t>(Materials::Air);
	}
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

struct BoxFiller
{
	uint8_t m_value;
	Math::Box3 m_box;
	void operator()(VoxelModel::ClumpDataAccessor& accessor, glm::vec3 clumpOrigin, glm::vec3 voxelSize, glm::vec3 voxelCenter)
	{
		const Math::Box3 clumpBox(clumpOrigin, clumpOrigin + (voxelSize * 2.0f));
		if (!m_box.Intersects(clumpBox))
		{
			return;
		}

		auto theClump = accessor.GetClump();
		for (int z = 0;z < 2;++z)
		{
			for (int y = 0;y < 2;++y)
			{
				for (int x = 0;x < 2;++x)
				{
					glm::vec3 vPos = VoxelPosition(clumpOrigin, voxelSize, voxelCenter, x, y, z);
					if (Box(vPos, m_box.Min(), m_box.Max(), 1))
					{
						theClump->VoxelAt(x, y, z) = m_value;
					}
				}
			}
		}
	}
};

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
					uint8_t outerWall = Box(vPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 8.0f, 128.0f), 1);
					outerWall += Box(vPos, glm::vec3(127.5f, 0.0f, 0.0f), glm::vec3(128.0f, 8.0f, 128.0f), 1);
					outerWall += Box(vPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(128.0f, 8.0f, 0.5f), 1);
					outerWall += Box(vPos, glm::vec3(0.0f, 0.0f, 127.5f), glm::vec3(128.0f, 8.0f, 128.0f), 1);
					outerWall += Box(vPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(128.0f, 0.125f, 128.0f), 1);
					if (outerWall != 0)
					{
						auto theClump = accessor.GetClump();
						theClump->VoxelAt(x, y, z) = static_cast<uint8_t>(Materials::OuterWall);
						continue;
					}

					// apply inner rooms as a simple grid
					glm::vec3 wallSpacePos = glm::mod(vPos, glm::vec3(16.0f, 8.0f, 16.0f));
					uint8_t innerWall = Box(wallSpacePos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 8.0f, 16.0f), 1);
					innerWall += Box(wallSpacePos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(16.0f, 8.0f, 0.5f), 1);
					if (innerWall != 0)
					{
						// subtract doors to inner walls
						uint8_t isDoor = Box(wallSpacePos, glm::vec3(6.0f, 0.125f, 0.0f), glm::vec3(10.0f, 6.0f, 0.5f), 1);
						isDoor += Box(wallSpacePos, glm::vec3(0.0f, 0.125f, 6.0f), glm::vec3(0.5f, 6.0f, 10.0f), 1);
						if (!isDoor)
						{
							// apply 'steel' interior
							uint8_t innerSteelWall = Box(wallSpacePos, glm::vec3(0.0f+0.125f, 0.0f, 0.0f), glm::vec3(0.25f + 0.125f, 8.0f, 16.0f), 1);
							innerSteelWall += Box(wallSpacePos, glm::vec3(0.0f, 0.0f, 0.0f + 0.125f), glm::vec3(16.0f, 8.0f, 0.5f-0.125f), 1);

							auto theClump = accessor.GetClump();
							theClump->VoxelAt(x, y, z) = innerSteelWall != 0 ? static_cast<uint8_t>(Materials::OuterWall) : static_cast<uint8_t>(Materials::Walls);
							continue;
						}
					}

					// apply carpets to each inner room
					if (Box(wallSpacePos, glm::vec3(2.0f, 0.125f, 2.0f), glm::vec3(14.0f, 0.25f, 14.0f), 1))
					{
						auto theClump = accessor.GetClump();
						theClump->VoxelAt(x, y, z) = static_cast<uint8_t>(Materials::Carpet);
						continue;
					}

					// apply floor if no carpet
					if (Box(wallSpacePos, glm::vec3(0.0f, 0.125f, 0.0f), glm::vec3(16.0f, 0.25f, 16.0f), 1))
					{
						auto theClump = accessor.GetClump();
						theClump->VoxelAt(x, y, z) = static_cast<uint8_t>(Materials::Floor);
						continue;
					}

					// apply pillars every two rooms
					glm::vec3 pillarSpacePos = glm::mod(vPos, glm::vec3(32.0f, 8.0f, 32.0f));
					uint8_t pillar = Box(pillarSpacePos, glm::vec3(23.5f, 0.0f, 23.5f), glm::vec3(24.5f, 0.5f, 24.5f), 1);
					pillar += Box(pillarSpacePos, glm::vec3(23.75f, 0.5f, 23.75f), glm::vec3(24.25f, 7.5f, 24.25f), 1);
					pillar += Box(pillarSpacePos, glm::vec3(23.5f, 7.5f, 23.5f), glm::vec3(24.5f, 8.0f, 24.5f), 1);
					if (pillar)
					{
						if (Box(pillarSpacePos, glm::vec3(23.75f+ 0.125f, 0.0f, 23.75f+ 0.125f), glm::vec3(24.25f- 0.125f, 8.0f, 24.25f- 0.125f), 1))
						{
							auto theClump = accessor.GetClump();
							theClump->VoxelAt(x, y, z) = static_cast<uint8_t>(Materials::OuterWall);
						}
						else
						{
							auto theClump = accessor.GetClump();
							theClump->VoxelAt(x, y, z) = static_cast<uint8_t>(Materials::Pillars);
						}
					}
				}
			}
		}
	}
};

void AppSkeleton::InitialiseFloor(std::shared_ptr<Core::Asset>& materialAsset)
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
	m_testFloor->Create(floorMaterials, glm::vec3(128.0f, 8.0f, 128.0f), 32);

	TestRoomBuilder valFiller;

#ifdef SDE_DEBUG
	m_testFloor->ModifyData(Math::Box3(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(16.0f, 8.0f, 16.0f)), valFiller);
#else
	m_testFloor->ModifyData(Math::Box3(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(128.0f, 8.0f, 128.0f)), valFiller);
#endif
}

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
	m_renderSystem->GetPass(m_forwardPassId).GetCamera().SetClipPlanes(0.1f, 256.0f);
	m_renderSystem->GetPass(m_forwardPassId).GetCamera().SetFOVAndAspectRatio(70.0f, 1280.0f / 720.0f);

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
#ifndef SDE_DEBUG
		SphereFiller sphere;
		sphere.m_value = 0;
		const int c_numHoles = 8;
		for (int i = 0;i < c_numHoles;++i)
		{
			sphere.m_radius = RandFloat(1.5f);
			sphere.m_center = glm::vec3(RandFloat(128.0f), RandFloat(8.0f), RandFloat(128.0f));
			m_testFloor->ModifyData(Math::Box3(sphere.m_center - sphere.m_radius, sphere.m_center + sphere.m_radius), sphere);
		}
#endif
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
	m_testFloor = nullptr;
	m_debugCameraController = nullptr;
}