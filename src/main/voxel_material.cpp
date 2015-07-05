#include "voxel_material.h"
#include "kernel/assert.h"

VoxelMaterialSet::VoxelMaterialSet()
{
	m_materials.resize(c_maxMaterials);
}

VoxelMaterialSet::~VoxelMaterialSet()
{
}

std::shared_ptr<Core::Asset> VoxelMaterialSet::GetRenderMaterialAsset()
{
	return m_renderMaterial;
}

void VoxelMaterialSet::SetRenderMaterialAsset(std::shared_ptr<Core::Asset>& asset)
{
	m_renderMaterial = asset;
}

void VoxelMaterialSet::SetMaterial(uint32_t materialID, const VoxelMaterial& newMaterial)
{
	SDE_ASSERT(materialID < c_maxMaterials);
	m_materials[materialID] = newMaterial;
}

const VoxelMaterial& VoxelMaterialSet::GetMaterial(uint32_t materialID) const
{
	SDE_ASSERT(materialID < c_maxMaterials);
	return m_materials[materialID];
}