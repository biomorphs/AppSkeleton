#include "floor.h"
#include "sde/job_system.h"
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
	m_sections.resize(sectionDimensions * sectionDimensions);
	m_dirtyMeshes.resize(sectionDimensions * sectionDimensions);
	m_voxelData.SetVoxelSize(glm::vec3(0.125f));	// All floors have constant voxel density of 8/meter

	// setup the mesh materials
	auto renderAsset = materials.GetRenderMaterialAsset();
	Render::MaterialAsset* mat = static_cast<Render::MaterialAsset*>(renderAsset.get());

	// setup the section descriptors
	for (int32_t z = 0; z < sectionDimensions; ++z)
	{
		for (int32_t x = 0; x < sectionDimensions; ++x)
		{
			auto& theSection = m_sections[x + (z * sectionDimensions)];
			const glm::vec3 boundsMin(x * m_sectionSize.x, 0.0f, z * m_sectionSize.z);
			theSection.m_bounds = Math::Box3(boundsMin, boundsMin + m_sectionSize);
			theSection.m_renderMesh.SetMaterial(mat->GetMaterial());
		}
	}
}

void Floor::Destroy()
{
	m_sections.clear();
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

	meshBuilder.CreateMesh(m_voxelData, m_materials, sectionBounds, m_sections[x + (z * m_sectionsPerSide)].m_renderMesh);
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

	// we need to know which sections are being modified
	glm::ivec3 sectionMin = glm::floor(minEditBounds / m_sectionSize);
	glm::ivec3 sectionMax = glm::ceil(maxEditBounds / m_sectionSize);

	// push the update jobs (one per section)
	for (int32_t z = sectionMin.z; z < sectionMax.z; ++z)
	{
		for (int32_t x = sectionMin.x; x < sectionMax.x; ++x)
		{
			// clamp modification bounds to the section
			Math::Box3 sectionBounds = m_sections[x + (z * m_sectionsPerSide)].m_bounds;
			auto updateJob = [this, sectionBounds, modifier, x, z]
			{
				// todo: if there are any jobs in progress for our data, we can't do anything
				m_sections[x + (z * m_sectionsPerSide)].m_jobCounter.Add(1);

				m_voxelData.IterateForArea(sectionBounds, VoxelModel::IteratorAccess::ReadWrite, modifier);
				m_dirtyMeshes[x + (z * m_sectionsPerSide)] = true;

				m_sections[x + (z * m_sectionsPerSide)].m_jobCounter.Add(-1);
			};
			m_jobSystem->PushJob(updateJob);
		}
	}
}

void Floor::Render(Render::RenderPass& targetPass)
{
	for (int32_t z = 0; z < m_sectionsPerSide; ++z)
	{
		for (int32_t x = 0; x < m_sectionsPerSide; ++x)
		{
			auto& theMesh = m_sections[x + (z * m_sectionsPerSide)].m_renderMesh;
			if (theMesh.GetStreams().size() > 0)
			{
				targetPass.AddInstance(&theMesh, glm::mat4());
			}
		}
	}
}