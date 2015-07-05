#pragma once

#include <glm.hpp>
#include <vector>
#include <memory>

namespace Core
{
	class Asset;
}

class VoxelMaterial
{
public:
	VoxelMaterial()
	: m_colour(1.0f,0.0f,1.0f,1.0f)
	{ 
	}

	inline const glm::vec4& Colour() const { return m_colour; }
	inline glm::vec4& Colour() { return m_colour; }

private:
	glm::vec4 m_colour;
};

// Each material set has c_maxMaterials entries. material 0 is air and is left empty
// One material asset per VoxelMaterialSet
class VoxelMaterialSet
{
public:
	VoxelMaterialSet();
	~VoxelMaterialSet();

	void SetRenderMaterialAsset(std::shared_ptr<Core::Asset>& asset);
	std::shared_ptr<Core::Asset> GetRenderMaterialAsset();

	void SetMaterial(uint32_t materialID, const VoxelMaterial& newMaterial);
	const VoxelMaterial& GetMaterial(uint32_t materialID) const;

private:
	static const uint32_t c_maxMaterials = 256;
	std::vector<VoxelMaterial> m_materials;
	std::shared_ptr<Core::Asset> m_renderMaterial;
};