#include "floor.h"
#include "render/mesh.h"
#include "render/material_asset.h"
#include "render/render_pass.h"
#include "math/intersections.h"
#include "voxel_mesh_builder.h"
#include "voxel_material.h"

Floor::Floor()
	: m_sectionsPerSide(0)
{

}

Floor::~Floor()
{
	Destroy();
}

void Floor::Create(VoxelMaterialSet& materials, const glm::vec3& floorSize, int32_t sectionDimensions)
{
	m_totalBounds = Math::Box3(glm::vec3(0.0f), floorSize);
	m_sectionSize = m_totalBounds.Size() / (float)sectionDimensions;
	m_sectionSize.y = floorSize.y;
	m_sectionsPerSide = sectionDimensions;
	m_materials = materials;
	m_sectionMeshes.resize(sectionDimensions * sectionDimensions);
	m_dirtyMeshes.resize(sectionDimensions * sectionDimensions);
	m_voxelData.SetVoxelSize(glm::vec3(0.125f));	// All floors have constant voxel density of 8/meter

	// get the mesh material
	auto renderAsset = materials.GetRenderMaterialAsset();
	Render::MaterialAsset* mat = static_cast<Render::MaterialAsset*>(renderAsset.get());

	for (int32_t z = 0; z < sectionDimensions; ++z)
	{
		for (int32_t x = 0; x < sectionDimensions; ++x)
		{
			auto& theMesh = m_sectionMeshes[x + (z * sectionDimensions)];
			theMesh = std::make_unique<Render::Mesh>();
			theMesh->SetMaterial(mat->GetMaterial());
		}
	}
}

void Floor::Destroy()
{
	m_sectionMeshes.clear();
	m_voxelData = VoxelModel();
}

bool Floor::RayIntersectsRoom(const glm::vec3& rayStart, const glm::vec3& rayEnd, float& tNear)
{
	return Math::RayIntersectsAAB(rayStart, rayEnd, m_totalBounds, tNear);
}

void Floor::RebuildDirtyMeshes()
{
	for (int32_t z = 0; z < m_sectionsPerSide; ++z)
	{
		for (int32_t x = 0; x < m_sectionsPerSide; ++x)
		{
			if (m_dirtyMeshes[x + (z*m_sectionsPerSide)])
			{
				RemeshSection(x, z);
				m_dirtyMeshes[x + (z*m_sectionsPerSide)] = false;
			}
		}
	}
}

void Floor::RemeshSection(int32_t x, int32_t z)
{
	SDE_ASSERT(x >= 0 && x < m_sectionsPerSide);
	SDE_ASSERT(z >= 0 && z < m_sectionsPerSide);
	VoxelMeshBuilder meshBuilder;
	glm::vec3 sectionBottomCorner = m_sectionSize * glm::vec3(x, 0, z);
	Math::Box3 sectionBounds(sectionBottomCorner, sectionBottomCorner + m_sectionSize);

	meshBuilder.CreateMesh(m_voxelData, m_materials, sectionBounds, *m_sectionMeshes[x + (z * m_sectionsPerSide)]);
}

void Floor::ModifyData(const Math::Box3& bounds, VoxelModel::ClumpIterator modifier)
{
	if (!bounds.Intersects(m_totalBounds))
	{
		return;
	}

	// clamp modification bounds to the floor bounds
	glm::vec3 minEditBounds = glm::clamp(bounds.Min(), m_totalBounds.Min(), m_totalBounds.Max());
	glm::vec3 maxEditBounds = glm::clamp(bounds.Max(), m_totalBounds.Min(), m_totalBounds.Max());

	// iterate using read-write permissions
	m_voxelData.IterateForArea(Math::Box3(minEditBounds, maxEditBounds), VoxelModel::IteratorAccess::ReadWrite, modifier);

	// now we need to mark intersecting sections as dirty
	glm::ivec3 sectionMin = glm::floor(minEditBounds / m_sectionSize);
	glm::ivec3 sectionMax = glm::ceil(maxEditBounds / m_sectionSize);

	for (int32_t z = sectionMin.z; z < sectionMax.z; ++z)
	{
		for (int32_t x = sectionMin.x; x < sectionMax.x; ++x)
		{
			m_dirtyMeshes[x + (z * m_sectionsPerSide)] = true;
		}
	}
}

void Floor::Render(Render::RenderPass& targetPass)
{
	for (int32_t z = 0; z < m_sectionsPerSide; ++z)
	{
		for (int32_t x = 0; x < m_sectionsPerSide; ++x)
		{
			auto& theMesh = m_sectionMeshes[x + (z * m_sectionsPerSide)];
			if (theMesh->GetStreams().size() > 0)
			{
				targetPass.AddInstance(theMesh.get(), glm::mat4());
			}
		}
	}
}