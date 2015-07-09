#pragma once

#include "kernel/base_types.h"
#include "vox/model.h"

class VoxelBlockAllocator
{
public:
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

typedef uint8_t VoxelData;
typedef Vox::Model<VoxelData, 32, VoxelBlockAllocator> VoxelModel;

// Base materials
enum class Materials : uint8_t
{
	Air,
	Walls,
	Floor,
	Carpet,
	Pillars,
	OuterWall,
};

inline uint8_t GetVoxelDamage(uint8_t v)
{
	return (v & 0xc0) >> 6;	// reserve top 2 bits for damage (0 - 3)
}

inline Materials GetVoxelMaterial(uint8_t v)
{
	return static_cast<Materials>(v & ~0xc0);
}

inline uint8_t PackVoxel(Materials material, uint8_t dmg)
{
	return static_cast<uint8_t>(material) | (dmg << 6);
}