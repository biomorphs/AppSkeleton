#pragma once

template<class ModelType>
class VoxelModelLoader
{
public:
	VoxelModelLoader();
	~VoxelModelLoader();

	typedef std::function<void(glm::ivec3)> OnBlockLoadedCallback;
	bool LoadFromFile(ModelType& srcModel, const char* filepath, const OnBlockLoadedCallback& callback);

private:
	void ParseBlock(ModelType& srcModel, size_t& readOffset, const OnBlockLoadedCallback& callback);
	std::vector<uint8_t> m_rawBuffer;
};

#include "vox_model_loader.inl"