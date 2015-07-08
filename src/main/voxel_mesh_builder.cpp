#include "voxel_mesh_builder.h"
#include "voxel_material.h"
#include "vox/greedy_quad_extractor.h"
#include "render/mesh_builder.h"

void VoxelMeshBuilder::BuildMeshData(const VoxelModel& sourceModel, const VoxelMaterialSet& materials, const Math::Box3& modelBounds, Render::MeshBuilder& targetMesh)
{
	// Extract quads using greedy mesher
	Vox::GreedyQuadExtractor<VoxelModel> extractor(sourceModel);
	extractor.ExtractQuads(modelBounds);

	if (extractor.Begin() == extractor.End())
	{
		return;
	}
	
	uint32_t posStream = targetMesh.AddVertexStream(3);
	uint32_t colourStream = targetMesh.AddVertexStream(4);
	uint32_t normalLookupStream = targetMesh.AddVertexStream(1);

	targetMesh.BeginChunk();
	for (auto q = extractor.Begin(); q != extractor.End(); ++q)
	{
		const auto& material = materials.GetMaterial(q->m_sourceData);
		const float normal = static_cast<float>(q->m_normal);
		const glm::vec4& colour = material.Colour();

		targetMesh.BeginTriangle();
		targetMesh.SetStreamData(posStream, q->m_vertices[0], q->m_vertices[1], q->m_vertices[2]);
		targetMesh.SetStreamData(colourStream, colour, colour, colour);
		targetMesh.SetStreamData(normalLookupStream, normal, normal, normal);
		targetMesh.EndTriangle();

		targetMesh.BeginTriangle();
		targetMesh.SetStreamData(posStream, q->m_vertices[0], q->m_vertices[2], q->m_vertices[3]);
		targetMesh.SetStreamData(colourStream, colour, colour, colour);
		targetMesh.SetStreamData(normalLookupStream, normal, normal, normal);
		targetMesh.EndTriangle();
	}
	targetMesh.EndChunk();
}