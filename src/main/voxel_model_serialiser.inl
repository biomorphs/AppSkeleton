#include "vox/model_data_reader.h"
#include "core/run_length_encoding.h"
#include "kernel/file_io.h"

enum VoxelModelFileVersions
{
	Version_BaseRLE,	// Basic RLE-encoding per-block
	Version_Current = Version_BaseRLE
};

struct ModelDataHeader
{
	char m_magic[8];
	uint32_t m_version;
	uint32_t m_blockCount;
};

struct ModelBlockHeader
{
	int32_t m_blockX;
	int32_t m_blockY;
	int32_t m_blockZ;
};

template<class ModelType>
VoxelModelSerialiser<ModelType>::VoxelModelSerialiser()
{

}

template<class ModelType>
VoxelModelSerialiser<ModelType>::~VoxelModelSerialiser()
{

}

template<class ModelType>
bool VoxelModelSerialiser<ModelType>::WriteBlockToFile(std::vector<uint8_t>& file, const glm::ivec3& blockIndex, typename const ModelType::BlockType* src)
{
	uint32_t dimensions = typename ModelType::BlockType::VoxelDimensions;
	std::vector<typename ModelType::BlockType::VoxelDataType> oneStride;	// Pass data to rle one stride of x axis at a time for speed
	oneStride.resize(dimensions);
	Core::RunLengthEncoder rle;
	bool isEmpty = true;
	const auto fileSizeBeforeRLE = file.size();	// we rewind if the block is empty, there's no need to store it

	ModelBlockHeader header;
	header.m_blockX = blockIndex.x;
	header.m_blockY = blockIndex.y;
	header.m_blockZ = blockIndex.z;
	file.insert(file.end(), (uint8_t*)&header, (uint8_t*)&header + sizeof(header));
	for (uint32_t z = 0; z < dimensions; ++z)
	{
		for (uint32_t y = 0; y < dimensions; ++y)
		{
			for (uint32_t x = 0; x < dimensions; ++x)
			{
				auto v = src->VoxelAt(x, y, z);
				oneStride[x] = v;
				isEmpty &= (v == 0);
			}
			rle.WriteData(reinterpret_cast<const uint8_t*>(oneStride.data()),
				oneStride.size() * sizeof(typename ModelType::BlockType::VoxelDataType),
				file);
		}
	}
	rle.Flush(file);

	if (isEmpty)
	{
		file.resize(fileSizeBeforeRLE);
		return false;
	}
	else
	{
		return true;
	}
}

template<class ModelType>
void VoxelModelSerialiser<ModelType>::WriteToFile(const ModelType& srcModel, const char* filepath)
{
	std::vector<uint8_t> rawData;
	rawData.resize(sizeof(ModelDataHeader));
	
	int32_t blocksSerialised = 0;
	glm::ivec3 blockStartIndices, blockEndIndices;
	srcModel.GetBlockIterationParameters(srcModel.GetTotalBounds(), blockStartIndices, blockEndIndices);

	for (int32_t blZ = blockStartIndices.z; blZ <= blockEndIndices.z; ++blZ)
	{
		for (int32_t blY = blockStartIndices.y; blY <= blockEndIndices.y; ++blY)
		{
			for (int32_t blX = blockStartIndices.x; blX <= blockEndIndices.x; ++blX)
			{
				glm::ivec3 blockCoords(blX, blY, blZ);
				auto thisBlock = srcModel.BlockAt(blockCoords);
				if (thisBlock != nullptr)
				{
					blocksSerialised += WriteBlockToFile(rawData, blockCoords, thisBlock) ? 1 : 0;
				}				
			}
		}
	}

	ModelDataHeader* header = reinterpret_cast<ModelDataHeader*>(rawData.data());
	strcpy_s(header->m_magic, "VoxM");
	header->m_version = Version_Current;
	header->m_blockCount = blocksSerialised;

	Kernel::FileIO::SaveBinaryFile(filepath, rawData);
}