#pragma once

#include "floor_stats.h"
#include "voxel_definitions.h"
#include "voxel_material.h"
#include "vox/model_area_data_writer.h"
#include "render/mesh.h"
#include "render/mesh_builder.h"
#include "math/box3.h"
#include "kernel/atomics.h"
#include "kernel/mutex.h"
#include <vector>

namespace Render
{
	class RenderPass;
	class Camera;
}

namespace SDE
{
	class JobSystem;
}

// Represents a single floor in a building. Floors are organised as a single voxel model,
// with a grid of sections representing individual meshes.
// All updates are async, and there is no access to internal data on the main thread
class Floor
{
public:
	Floor();
	~Floor();

	void Create(SDE::JobSystem* jobSystem, VoxelMaterialSet& materials, const glm::vec3& floorSize, int32_t sectionDimensions);
	void Destroy();
	void RebuildDirtyMeshes();
	void Update();
	void Render(Render::Camera& camera, Render::RenderPass& targetPass);
	void DisplayDebugGui(DebugGui::DebugGuiSystem& gui);

	// Async stuff
	bool LoadFile(const char* filename);
	void SaveNow(const char* filename);
	void ModifyData(const Math::Box3& bounds, const Vox::ModelAreaDataWriter<VoxelModel>::AreaCallback& modifier);
	void ModifyDataAndSave(const Math::Box3& bounds, const Vox::ModelAreaDataWriter<VoxelModel>::AreaCallback& modifier, const char* filename);

	// Test!
	inline VoxelModel& GetModel() { return m_voxelData; }

private:
	struct SectionDesc
	{
		Math::Box3 m_bounds;
		Render::Mesh m_renderMesh;
		Kernel::AtomicInt32 m_updateJobCounter;	// How many jobs are acting on this data
		Kernel::AtomicInt32 m_updatesPending;	// How many update jobs have been queued. if it hits 0, we are safe to mesh it		
	};

	void RemeshSection(int32_t x, int32_t z);
	void SubmitUpdateJob(const Math::Box3& updateBounds, int32_t x, int32_t z, const Vox::ModelAreaDataWriter<VoxelModel>::AreaCallback& iterator);
	void SubmitRemeshJob(const Math::Box3& updateBounds, int32_t x, int32_t z);
	SectionDesc& GetSection(int32_t x, int32_t z);
	void AddSectionMeshResult(int32_t x, int32_t z, Render::MeshBuilder& result);

	Kernel::Mutex m_updatedMeshesLock;		// Meshing results protected by mutex (since main thread needs them)
	std::unordered_map<int32_t, Render::MeshBuilder> m_updatedMeshes;	// map of sectionindex -> mesh builder results
	std::vector<SectionDesc> m_sections;
	Math::Box3 m_totalBounds;
	glm::vec3 m_sectionSize;
	int32_t m_sectionsPerSide;
	VoxelModel m_voxelData;
	VoxelMaterialSet m_materials;
	SDE::JobSystem* m_jobSystem;
	Kernel::AtomicInt32 m_isSaving;
	Kernel::AtomicInt32 m_isLoading;
	Kernel::AtomicInt32 m_loadInProgress;
	Kernel::AtomicInt32 m_totalWritesPending;
	Kernel::AtomicInt32 m_totalVbBytes;
	std::string m_saveFilename;
	std::string m_loadFilename;
	FloorStats m_stats;
};