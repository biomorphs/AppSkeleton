#include "vox_model_fileformat.h"
#include "vox/model_data_writer.h"
#include "core/run_length_encoding.h"
#include "kernel/file_io.h"

template<class ModelType>
VoxelModelLoader<ModelType>::VoxelModelLoader()
{

}

template<class ModelType>
VoxelModelLoader<ModelType>::~VoxelModelLoader()
{
}

template<class ModelType>
void VoxelModelLoader<ModelType>::ParseBlock(ModelType& srcModel, size_t& readOffset, const OnBlockLoadedCallback& callback)
{
	const uint32_t dimensions = typename ModelType::BlockType::VoxelDimensions;

	ModelBlockHeader* blockHeader = reinterpret_cast<ModelBlockHeader*>(m_rawBuffer.data() + readOffset);
	readOffset += sizeof(ModelBlockHeader);

	Vox::ModelDataWriter<ModelType> dataWriter(srcModel);
	glm::ivec3 blockIndex(blockHeader->m_blockX, blockHeader->m_blockY, blockHeader->m_blockZ);

	// Now decode the entire block at once
	Core::RunLengthDecoder rld;
	std::vector<uint8_t> decodedBlock;
	decodedBlock.reserve(blockHeader->m_dataSize);
	rld.ReadData(m_rawBuffer.data() + readOffset, blockHeader->m_dataSize, decodedBlock);
	readOffset += blockHeader->m_dataSize;

	auto vData = reinterpret_cast<typename ModelType::BlockType::VoxelDataType*>(decodedBlock.data());
	SDE_ASSERT(decodedBlock.size() == sizeof(typename ModelType::BlockType::VoxelDataType) * dimensions * dimensions * dimensions);

	glm::ivec3 voxelIndex;
	for (uint32_t z = 0; z < dimensions; ++z)
	{
		voxelIndex.z = z;
		for (uint32_t y = 0; y < dimensions; ++y)
		{
			voxelIndex.y = y;
			for (uint32_t x = 0; x < dimensions; ++x)
			{
				voxelIndex.x = x;
				auto v = vData++;
				dataWriter.WriteVoxel(blockIndex, voxelIndex, *v);
			}
		}
	}
	callback(blockIndex);
}

template<class ModelType>
bool VoxelModelLoader<ModelType>::LoadFromFile(ModelType& srcModel, const char* filepath, const OnBlockLoadedCallback& callback)
{
	uint32_t dimensions = typename ModelType::BlockType::VoxelDimensions;
	if (!Kernel::FileIO::LoadBinaryFile(filepath, m_rawBuffer))
	{
		return false;
	}
	SDE_ASSERT(m_rawBuffer.size() > sizeof(ModelDataHeader));

	ModelDataHeader* header = (ModelDataHeader*)m_rawBuffer.data();
	if (strcmp(header->m_magic, "VoxM") != 0)
	{
		SDE_ASSERT("Wrong format");
		return false;
	}
	if (header->m_version != Version_Current)
	{
		SDE_ASSERT("Old version");
		return false;
	}
	if (header->m_blockDimensions != typename ModelType::BlockType::VoxelDimensions)
	{
		SDE_ASSERT("Incompatible voxel data dimensions");
		return false;
	}
	srcModel.SetVoxelSize(glm::vec3(header->m_voxelSize[0], header->m_voxelSize[1], header->m_voxelSize[2]));
	srcModel.PreallocateMemory(Math::Box3(glm::vec3(header->m_totalBounds[0], header->m_totalBounds[1], header->m_totalBounds[2]),
			glm::vec3(header->m_totalBounds[3], header->m_totalBounds[4], header->m_totalBounds[5])));		

	size_t readOffset = sizeof(ModelDataHeader);
	for (uint32_t b = 0; b < header->m_blockCount; ++b)
	{
		ParseBlock(srcModel, readOffset, callback);
	}
	SDE_ASSERT(readOffset <= m_rawBuffer.size());

	return true;
}


