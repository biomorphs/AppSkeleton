#pragma once
#include "kernel/base_types.h"

enum VoxelModelFileVersions
{
	Version_BaseRLE,	// Basic RLE-encoding per-block
	Version_Current = Version_BaseRLE
};

struct ModelDataHeader
{
	char m_magic[8];
	uint32_t m_version;
	uint32_t m_blockDimensions;
	uint32_t m_blockCount;
	float m_voxelSize[3];
	float m_totalBounds[6];
};

struct ModelBlockHeader
{
	int32_t m_blockX;
	int32_t m_blockY;
	int32_t m_blockZ;
	uint32_t m_dataSize;
};