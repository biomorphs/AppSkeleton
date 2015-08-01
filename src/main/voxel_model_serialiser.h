#pragma once

template<class ModelType>
class VoxelModelSerialiser
{
public:
	VoxelModelSerialiser();
	~VoxelModelSerialiser();

	void WriteToFile(const ModelType& srcModel, const char* filepath);
private:
	bool WriteBlockToFile(std::vector<uint8_t>& file, const glm::ivec3& blockIndex, typename const ModelType::BlockType* src);
};

#include "voxel_model_serialiser.inl"