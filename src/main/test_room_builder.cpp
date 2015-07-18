#include "test_room_builder.h"
#include "procedural_geometry.h"
#include <glm.hpp>
#include <algorithm>

inline uint8_t Pillar(const glm::vec3& vox, const glm::vec3& bl, const glm::vec3& tr)
{
	if (Box(vox, bl, tr) > 0.0f)
	{
		return 0;
	}

	if (Box(vox, glm::vec3(bl.x + 0.35f, bl.y, bl.z + 0.35f), glm::vec3(tr.x - 0.35f, tr.y, tr.z - 0.35f)) <= 0.0f)
	{
		return static_cast<uint8_t>(Materials::OuterWall);
	}

	float pillar = Box(vox, bl, glm::vec3(tr.x, 0.5f, tr.z));
	pillar = std::min(pillar, Box(vox, glm::vec3(bl.x, tr.y - 0.5f, bl.z), tr));
	pillar = std::min(pillar, Box(vox, bl + glm::vec3(0.25f), tr - glm::vec3(0.25f)));
	if (pillar <= 0.0f)
	{
		return static_cast<uint8_t>(Materials::Pillars);
	}
	return 0;
}

uint8_t RoomWithRug(const glm::vec3& vox, const glm::vec3& bl, const glm::vec3& tr, float rg)
{
	const float c_iwt = 0.125f;		// inner wall thickness
	if (Box(vox, bl, tr) > 0.0f)
	{
		return -1;
	}
	else
	{
		// Add Interior walls with thickness of 1 voxel
		float innerWall = Box(vox, bl + glm::vec3(0.0f, 0.125f, 0.0f), tr);
		float innerSpace = Box(vox, bl + glm::vec3(c_iwt, 0.0f, c_iwt), tr - glm::vec3(c_iwt, 0.0f, c_iwt));
		if (innerWall <= 0.0f && innerSpace > 0.0f)
		{
			return (uint8_t)Materials::Walls;
		}

		// Add floor + carpet
		float floor = Box(vox, bl, glm::vec3(tr.x, bl.y + 0.125f, tr.z));
		if (floor <= 0.0f)
		{
			return (uint8_t)Materials::OuterWall;
		}

		float rug = Box(vox, bl + glm::vec3(rg, 0.125f, rg), glm::vec3(tr.x - rg, bl.y + 0.25f, tr.z - rg));
		if (rug <= 0.0f)
		{
			return (uint8_t)Materials::Carpet;
		}

		float carpet = Box(vox, bl + glm::vec3(0.0f, 0.125f, 0.0f), glm::vec3(tr.x, bl.y + 0.25f, tr.z));
		if (carpet <= 0.0f)
		{
			return (uint8_t)Materials::Floor;
		}

		return (uint8_t)Materials::Air;
	}
}

uint8_t Room(const glm::vec3& vox, const glm::vec3& bl, const glm::vec3& tr)
{
	const float c_iwt = 0.125f;		// inner wall thickness
	if (Box(vox, bl, tr) > 0.0f)
	{
		return -1;
	}
	else
	{
		// Add Interior walls with thickness of 1 voxel
		float innerWall = Box(vox, bl + glm::vec3(0.0f,0.125f,0.0f), tr);
		float innerSpace = Box(vox, bl + glm::vec3(c_iwt, 0.0f, c_iwt), tr - glm::vec3(c_iwt, 0.0f, c_iwt));
		if (innerWall <= 0.0f && innerSpace > 0.0f)
		{
			return (uint8_t)Materials::Walls;
		}

		// Add floor + carpet
		float floor = Box(vox, bl, glm::vec3(tr.x, bl.y + 0.125f, tr.z));
		if (floor <= 0.0f)
		{
			return (uint8_t)Materials::OuterWall;
		}

		float carpet = Box(vox, bl + glm::vec3(0.0f,0.125f,0.0f), glm::vec3(tr.x, bl.y + 0.25f, tr.z));
		if (carpet <= 0.0f)
		{
			return (uint8_t)Materials::Floor;
		}

		return (uint8_t)Materials::Air;
	}
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

				// fill with outter wall, subtract details
				float outerWall = Box(vPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(128.0f, 8.0f, 128.0f));
				if (outerWall <= 0.0f)
				{
					areaParams.WriteVoxel(vx, vy, vz, (uint8_t)Materials::OuterWall);
				}

				// Corridoors
				uint8_t corridoor = Room(vPos, glm::vec3(20.0f, 0.0f, 0.25f), glm::vec3(28.0f, 8.0f, 127.75f));
				if (corridoor != (uint8_t)-1)
				{
					areaParams.WriteVoxel(vx, vy, vz, corridoor);
					continue;
				}
				corridoor = Room(vPos, glm::vec3(100.0f, 0.0f, 0.25f), glm::vec3(108.0f, 8.0f, 127.75f));
				if (corridoor != (uint8_t)-1)
				{
					areaParams.WriteVoxel(vx, vy, vz, corridoor);
					continue;
				}

				// middle arena
				uint8_t arena = RoomWithRug(vPos, glm::vec3(28.25f, 0.0f, 34.0f), glm::vec3(99.75f, 8.0f, 94.0f), 6.0f);
				if (arena != (uint8_t)-1)
				{
					areaParams.WriteVoxel(vx, vy, vz, arena);
					continue;
				}

				// bottom rooms
				uint8_t r = Room(vPos, glm::vec3(28.25f, 0.0f, 0.25f), glm::vec3(63.75f, 8.0f, 33.75f));
				if (r != (uint8_t)-1)
				{
					areaParams.WriteVoxel(vx, vy, vz, r);
					continue;
				}
				r = Room(vPos, glm::vec3(64.25f, 0.0f, 0.25f), glm::vec3(99.75f, 8.0f, 33.75f));
				if (r != (uint8_t)-1)
				{
					areaParams.WriteVoxel(vx, vy, vz, r);
					continue;
				}

				// top rooms
				r = Room(vPos, glm::vec3(28.25f, 0.0f, 94.25f), glm::vec3(63.75f, 8.0f, 127.75f));
				if (r != (uint8_t)-1)
				{
					areaParams.WriteVoxel(vx, vy, vz, r);
					continue;
				}
				r = Room(vPos, glm::vec3(64.25f, 0.0f, 94.25f), glm::vec3(99.75f, 8.0f, 127.75f));
				if (r != (uint8_t)-1)
				{
					areaParams.WriteVoxel(vx, vy, vz, r);
					continue;
				}

				// Side passages
				r = Room(vPos, glm::vec3(0.25f, 0.0f, 30.0f), glm::vec3(19.75f, 8.0f, 36.0f));
				if (r != (uint8_t)-1)
				{
					areaParams.WriteVoxel(vx, vy, vz, r);
					continue;
				}
				r = Room(vPos, glm::vec3(0.25f, 0.0f, 92.0f), glm::vec3(19.75f, 8.0f, 98.0f));
				if (r != (uint8_t)-1)
				{
					areaParams.WriteVoxel(vx, vy, vz, r);
					continue;
				}
				r = Room(vPos, glm::vec3(108.25f, 0.0f, 30.0f), glm::vec3(127.75f, 8.0f, 36.0f));
				if (r != (uint8_t)-1)
				{
					areaParams.WriteVoxel(vx, vy, vz, r);
					continue;
				}
				r = Room(vPos, glm::vec3(108.25f, 0.0f, 92.0f), glm::vec3(127.75f, 8.0f, 98.0f));
				if (r != (uint8_t)-1)
				{
					areaParams.WriteVoxel(vx, vy, vz, r);
					continue;
				}	

				// Corner rooms
				r = Room(vPos, glm::vec3(0.25f, 0.0f, 0.25f), glm::vec3(19.75f, 8.0f, 29.75f));
				if (r != (uint8_t)-1)
				{
					areaParams.WriteVoxel(vx, vy, vz, r);
					continue;
				}
				r = Room(vPos, glm::vec3(0.25f, 0.0f, 98.25f), glm::vec3(19.75f, 8.0f, 127.75f));
				if (r != (uint8_t)-1)
				{
					areaParams.WriteVoxel(vx, vy, vz, r);
					continue;
				}
				r = Room(vPos, glm::vec3(108.25f, 0.0f, 0.25f), glm::vec3(127.75f, 8.0f, 29.75f));
				if (r != (uint8_t)-1)
				{
					areaParams.WriteVoxel(vx, vy, vz, r);
					continue;
				}
				r = Room(vPos, glm::vec3(108.25f, 0.0f, 98.25f), glm::vec3(127.75f, 8.0f, 127.75f));
				if (r != (uint8_t)-1)
				{
					areaParams.WriteVoxel(vx, vy, vz, r);
					continue;
				}

				// Side rooms
				r = Room(vPos, glm::vec3(0.25f, 0.0f, 36.25f), glm::vec3(19.75f, 8.0f, 91.75f));
				if (r != (uint8_t)-1)
				{
					areaParams.WriteVoxel(vx, vy, vz, r);
					continue;
				}
				r = Room(vPos, glm::vec3(108.25f, 0.0f, 36.25f), glm::vec3(127.75f, 8.0f, 91.75f));
				if (r != (uint8_t)-1)
				{
					areaParams.WriteVoxel(vx, vy, vz, r);
				}
			}
		}
	}
}