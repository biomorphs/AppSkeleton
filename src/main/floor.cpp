#include "floor.h"
#include "sde/job_system.h"
#include "render/mesh.h"
#include "render/material_asset.h"
#include "render/mesh_builder.h"
#include "render/render_pass.h"
#include "render/camera.h"
#include "math/intersections.h"
#include "voxel_mesh_builder.h"
#include "voxel_material.h"
#include "voxel_model_serialiser.h"
#include "vox_model_loader.h"

static const glm::vec3 c_floorTotalSize(128.0f);

Floor::Floor()
	: m_sectionsPerSide(0)
	, m_isSaving(0)
	, m_isLoading(0)
	, m_totalWritesPending(0)
	, m_loadInProgress(0)
	, m_totalVbBytes(0)
{
}

Floor::~Floor()
{
	Destroy();
}

Floor::SectionDesc& Floor::GetSection(int32_t x, int32_t z)
{
	return m_sections[x + (z * m_sectionsPerSide)];
}

void Floor::DisplayDebugGui(DebugGui::DebugGuiSystem& gui)
{
	m_stats.UpdateStats(m_totalBounds, m_sectionSize, m_totalWritesPending.Get(), m_totalVbBytes.Get(), m_voxelData.TotalVoxelMemory());
	m_stats.DisplayDebugGui(gui);
}

bool Floor::LoadFile(const char* filename)
{
	m_loadFilename = filename;
	m_isLoading = true;
	return true;
}

void Floor::Create(SDE::JobSystem* jobSystem, VoxelMaterialSet& materials, const glm::vec3& floorSize, int32_t sectionDimensions)
{
	SDE_ASSERT(jobSystem != nullptr);
	m_totalBounds = Math::Box3(glm::vec3(0.0f), floorSize);
	m_sectionSize = m_totalBounds.Size() / (float)sectionDimensions;
	m_sectionSize.y = floorSize.y;
	m_sectionsPerSide = sectionDimensions;
	m_materials = materials;
	m_sections.resize(sectionDimensions * sectionDimensions);
	m_voxelData.SetVoxelSize(glm::vec3(0.125f));	// All floors have constant voxel density of 8/meter
	m_jobSystem = jobSystem;

	// setup the mesh materials
	auto renderAsset = materials.GetRenderMaterialAsset();
	Render::MaterialAsset* mat = static_cast<Render::MaterialAsset*>(renderAsset.get());

	// setup the section descriptors
	for (int32_t z = 0; z < sectionDimensions; ++z)
	{
		for (int32_t x = 0; x < sectionDimensions; ++x)
		{
			auto& theSection = GetSection(x, z);
			const glm::vec3 boundsMin(x * m_sectionSize.x, 0.0f, z * m_sectionSize.z);
			theSection.m_bounds = Math::Box3(boundsMin, boundsMin + m_sectionSize);
			theSection.m_renderMesh.SetMaterial(mat->GetMaterial());
		}
	}

	// We get away with being lockless by ensuring the voxel model data *structure*
	// does not change during async calls (i.e. no new blocks should be allocated)
	m_voxelData.PreallocateMemory(m_totalBounds);
}

void Floor::Destroy()
{
	m_sections.clear();
	m_voxelData = VoxelModel();
}

void Floor::RebuildDirtyMeshes()
{
	// we move the entire result data out, so we keep the lock for as little time
	// as possible
	std::unordered_map<int32_t, Render::MeshBuilder> buildResults;
	{
		Kernel::ScopedMutex lock(m_updatedMeshesLock);
		buildResults = std::move(m_updatedMeshes);
	}

	// now we're safe to remesh the results
	for (auto& it : buildResults)
	{
		// Rebuild the section index
		int32_t sectionX = it.first % m_sectionsPerSide;
		int32_t sectionY = (it.first - sectionX) / m_sectionsPerSide;

		auto& section = GetSection(sectionX, sectionY);

		m_totalVbBytes.Add(-(int32_t)section.m_renderMesh.TotalVertexBufferBytes());

		// Update the section render mesh
		it.second.CreateMesh(section.m_renderMesh, 1024 * 32);

		m_totalVbBytes.Add((int32_t)section.m_renderMesh.TotalVertexBufferBytes());
	}
}

void Floor::AddSectionMeshResult(int32_t x, int32_t z, Render::MeshBuilder& result)
{
	int32_t sectionResultIndex = x + (z * m_sectionsPerSide);
	{
		Kernel::ScopedMutex lock(m_updatedMeshesLock);
		m_updatedMeshes[sectionResultIndex] = std::move(result);
	}
}

void Floor::RemeshSection(int32_t x, int32_t z)
{
	SDE_ASSERT(x >= 0 && x < m_sectionsPerSide);
	SDE_ASSERT(z >= 0 && z < m_sectionsPerSide);

	// This assumes nobody else is touching this section, be careful!
	auto& thisSection = GetSection(x, z);
	
	// We basically do everything but actually update the gpu data (it must happen in the main thread)
	Render::MeshBuilder meshBuilder;
	VoxelMeshBuilder voxelMeshBuilder;

	voxelMeshBuilder.BuildMeshData(m_voxelData, m_materials, thisSection.m_bounds, meshBuilder);

	if (meshBuilder.HasData())
	{
		AddSectionMeshResult(x, z, meshBuilder);
	}
}

void Floor::SubmitRemeshJob(const Math::Box3& updateBounds, int32_t x, int32_t z)
{
	auto updateJob = [this, updateBounds, x, z]
	{
		auto& thisSection = GetSection(x, z);
		RemeshSection(x, z);
	};

 	m_jobSystem->PushJob(updateJob);
}

void Floor::SubmitUpdateJob(const Math::Box3& updateBounds, int32_t x, int32_t z, const Vox::ModelAreaDataWriter<VoxelModel>::AreaCallback& iterator)
{
	auto updateJob = [this, updateBounds, iterator, x, z]
	{
		auto& thisSection = GetSection(x, z);

		if (!thisSection.m_updateJobCounter.CAS(0,1))	// If there are update jobs currently running, we wait
		{
			thisSection.m_updatesPending.Add(-1);
			m_totalWritesPending.Add(-1);
			SubmitUpdateJob(updateBounds, x, z, iterator);
		}
		else
		{
			Vox::ModelAreaDataWriter<VoxelModel> areaWriter(m_voxelData);
			areaWriter.WriteArea(updateBounds, iterator);

			if (thisSection.m_updatesPending.Add(-1) == 1)	// If pending updates = 1, that's us, so we will now remesh
			{
				RemeshSection(x, z);
			}

			thisSection.m_updateJobCounter.Add(-1);	// we're done, someone else can update data now
			m_totalWritesPending.Add(-1);
		}
	};

	m_totalWritesPending.Add(1);
	GetSection(x, z).m_updatesPending.Add(1);		// Keep track of how many updates are queued
	m_jobSystem->PushJob(updateJob);
}

void Floor::SaveNow(const char* filename)
{
	SDE_ASSERT(m_isSaving.Get() == 0, "Dont overlap saves");
	m_saveFilename = filename;
	m_isSaving = 1;
}

void Floor::ModifyDataAndSave(const Math::Box3& bounds, const Vox::ModelAreaDataWriter<VoxelModel>::AreaCallback& modifier, const char* filename)
{
	if (!bounds.Intersects(m_totalBounds))
	{
		return;
	}

	SDE_ASSERT(m_isSaving.Get() == 0, "Dont overlap saves");
	
	m_saveFilename = filename;
	ModifyData(bounds, modifier);
	m_isSaving = 1;
}

void Floor::ModifyData(const Math::Box3& bounds, const Vox::ModelAreaDataWriter<VoxelModel>::AreaCallback& modifier)
{
	if (!bounds.Intersects(m_totalBounds))
	{
		return;
	}

	if (m_isSaving.Get() == 1)	// No other modifications allowed
	{
		return;
	}

	if (m_isLoading.Get() == 1)
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
			Math::Box3 sectionBounds = GetSection(x,z).m_bounds;
			sectionBounds.Min() = glm::max(sectionBounds.Min(), minEditBounds);
			sectionBounds.Max() = glm::min(sectionBounds.Max(), maxEditBounds);
			SubmitUpdateJob(sectionBounds, x, z, modifier);
		}
	}
}

void Floor::Update()
{
	if (m_isSaving.Get()==1)
	{
		if (m_totalWritesPending.Get() == 0)
		{
			// We will now issue a saving job.
			auto savingJob = [this]()
			{
				VoxelModelSerialiser<VoxelModel> serialiser;
				serialiser.WriteToFile(m_voxelData, m_saveFilename.c_str());
			};
			m_jobSystem->PushJob(savingJob, "Floor::Save");
			m_isSaving.Set(0);
		}
	}
	else if (m_isLoading.Get() == 1)
	{
		if (m_totalWritesPending.Get() == 0)
		{
			auto loadingJob = [this]()
			{
				VoxelModelLoader<VoxelModel> loader;
				auto bounds = m_voxelData.GetTotalBounds();
				m_voxelData.RemoveAllBlocks();
				loader.LoadFromFile(m_voxelData, m_loadFilename.c_str(), [](glm::ivec3 blockIndex)
				{
				});
				for (int32_t z = 0; z < m_sectionsPerSide; ++z)
				{
					for (int32_t x = 0; x < m_sectionsPerSide; ++x)
					{
						Math::Box3 sectionBounds;
						sectionBounds.Min() = glm::vec3(x * m_sectionSize.x, m_sectionSize.y, z * m_sectionSize.z);
						sectionBounds.Max() = sectionBounds.Min() + m_sectionSize;
						SubmitRemeshJob(sectionBounds, x, z);
					}
				}
			};	
			m_jobSystem->PushJob(loadingJob, "Floor::Load");
			m_loadInProgress.Add(1);
			m_isLoading.Set(0);
		}
	}
}

void Floor::Render(Render::Camera& camera, Render::RenderPass& targetPass)
{
	RebuildDirtyMeshes();
	for (int32_t z = 0; z < m_sectionsPerSide; ++z)
	{
		for (int32_t x = 0; x < m_sectionsPerSide; ++x)
		{		
			auto& theMesh = GetSection(x,z).m_renderMesh;
			if (theMesh.GetStreams().size() > 0)
			{
				const glm::mat4 mvp = camera.ProjectionMatrix() * camera.ViewMatrix();
				
				Render::UniformBuffer instanceUniforms;
				instanceUniforms.SetValue("MVP", mvp);

				targetPass.AddInstance(&theMesh, std::move(instanceUniforms));
			}
		}
	}
}