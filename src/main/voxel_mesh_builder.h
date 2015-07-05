#pragma once

#include "voxel_definitions.h"
#include "math/box3.h"

namespace Render
{
	class Mesh;
}

class VoxelMaterialSet;

class VoxelMeshBuilder
{
public:
	void CreateMesh(const VoxelModel& sourceModel, const VoxelMaterialSet& materials, const Math::Box3& modelBounds, Render::Mesh& targetMesh);
};