#pragma once

#include <stdint.h>
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