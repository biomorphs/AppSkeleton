#include "test_room_builder.h"
#include "procedural_geometry.h"
#include <glm.hpp>
#include <algorithm>

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

void TestRoomBuilder::operator()(Vox::ModelAreaDataWriterParams<VoxelModel>& areaParams)
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
				innerWall = std::min(innerWall, Box(wallSpacePos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(16.0f, 8.0f, 0.5f)));
				if (innerWall <= 0.0f)
				{
					float door = Box(wallSpacePos, glm::vec3(6.0f, 0.25f, 0.0f), glm::vec3(10.0f, 6.0f, 0.5f));
					door = std::min(door, Box(wallSpacePos, glm::vec3(0.0f, 0.125f, 6.0f), glm::vec3(0.5f, 6.0f, 10.0f)));
					if (door > 0.0f)
					{
						float innerDoor = Box(wallSpacePos, glm::vec3(5.75f, 0.25f, 0.0f), glm::vec3(10.25f, 6.25f, 0.5f));
						innerDoor = std::min(innerDoor, Box(wallSpacePos, glm::vec3(0.0f, 0.125f, 5.75f), glm::vec3(0.5f, 6.25f, 10.25f)));
						if (innerDoor <= 0.0f)
						{
							areaParams.WriteVoxel(vx, vy, vz, static_cast<uint8_t>(Materials::Walls));
						}
						else
						{
							float innerSteelWall = Box(wallSpacePos, glm::vec3(0.0f + 0.125f, 0.0f, 0.0f), glm::vec3(0.25f + 0.125f, 8.0f, 16.0f));
							innerSteelWall = std::min(innerSteelWall, Box(wallSpacePos, glm::vec3(0.0f, 0.0f, 0.0f + 0.125f), glm::vec3(16.0f, 8.0f, 0.5f - 0.125f)));
							areaParams.WriteVoxel(vx, vy, vz, innerSteelWall <= 0.0f ? static_cast<uint8_t>(Materials::OuterWall) : static_cast<uint8_t>(Materials::Walls));
						}
					}
					else
					{
						
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