#pragma once

#include "voxel_definitions.h"
#include "math/box3.h"

namespace Render
{
	class MeshBuilder;
}

class VoxelMaterialSet;

class VoxelMeshBuilder
{
public:
	// Populates a MeshBuilder with all the data required to push to the gpu
	void BuildMeshData(const VoxelModel& sourceModel, const VoxelMaterialSet& materials, const Math::Box3& modelBounds, Render::MeshBuilder& targetMesh);
};