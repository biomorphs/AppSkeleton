#pragma once

#include "voxel_definitions.h"
#include "voxel_material.h"
#include "render/mesh.h"
#include "math/box3.h"
#include "kernel/atomics.h"
#include <vector>
#include <memory>

namespace Render
{
	class RenderPass;
}

namespace SDE
{
	class JobSystem;
}

// Represents a single floor in a building
class Floor
{
public:
	Floor();
	~Floor();

	void SetJobSystem(SDE::JobSystem* jobSystem) { m_jobSystem = jobSystem; }
	void RebuildDirtyMeshes();
	void Render(Render::RenderPass& targetPass);
	void Create(VoxelMaterialSet& materials, const glm::vec3& floorSize, int32_t sectionDimensions);
	void Destroy();
	void ModifyData(const Math::Box3& bounds, VoxelModel::ClumpIterator modifier);

	bool RayIntersectsRoom(const glm::vec3& rayStart, const glm::vec3& rayEnd, float& tNear);

private:
	void RemeshSection(int32_t x, int32_t z);

	struct SectionDesc
	{
		Math::Box3 m_bounds;
		Render::Mesh m_renderMesh;
		Kernel::AtomicInt32 m_jobCounter;	// How many jobs are acting on this data
	};

	Math::Box3 m_totalBounds;
	glm::vec3 m_sectionSize;
	int32_t m_sectionsPerSide;
	std::vector<bool> m_dirtyMeshes;
	std::vector<SectionDesc> m_sections;
	VoxelModel m_voxelData;
	VoxelMaterialSet m_materials;
	SDE::JobSystem* m_jobSystem;
};