#pragma once

#include "voxel_definitions.h"
#include "vox/model_area_data_writer.h"
#include "kernel/base_types.h"

struct TestRoomBuilder
{
	void operator()(Vox::ModelAreaDataWriterParams<VoxelModel>& areaParams);
};