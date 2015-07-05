#pragma once

#include "voxel_definitions.h"
#include "voxel_material.h"
#include "math/box3.h"
#include <vector>
#include <memory>

namespace Render
{
	class Mesh;
	class RenderPass;
}

// Represents a single floor in a building
class Floor
{
public:
	Floor();
	~Floor();

	void RebuildDirtyMeshes();
	void Render(Render::RenderPass& targetPass);
	void Create(VoxelMaterialSet& materials, const glm::vec3& floorSize, int32_t sectionDimensions);
	void Destroy();
	void ModifyData(const Math::Box3& bounds, VoxelModel::ClumpIterator modifier);

	bool RayIntersectsRoom(const glm::vec3& rayStart, const glm::vec3& rayEnd, float& tNear);

private:
	void RemeshSection(int32_t x, int32_t z);

	Math::Box3 m_totalBounds;
	glm::vec3 m_sectionSize;
	int32_t m_sectionsPerSide;
	VoxelModel m_voxelData;
	VoxelMaterialSet m_materials;
	std::vector<std::unique_ptr<Render::Mesh>> m_sectionMeshes;
	std::vector<bool> m_dirtyMeshes;
};