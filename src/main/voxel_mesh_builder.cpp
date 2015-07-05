#include "voxel_mesh_builder.h"
#include "voxel_material.h"
#include "vox/greedy_quad_extractor.h"
#include "render/mesh_builder.h"

void VoxelMeshBuilder::CreateMesh(const VoxelModel& sourceModel, const VoxelMaterialSet& materials, const Math::Box3& modelBounds, Render::Mesh& targetMesh)
{
	// Extract quads using greedy mesher
	Vox::GreedyQuadExtractor<VoxelModel> extractor(sourceModel);
	extractor.ExtractQuads(modelBounds);

	if (extractor.Begin() == extractor.End())
	{
		return;
	}

	Render::MeshBuilder meshBuilder;
	uint32_t posStream = meshBuilder.AddVertexStream(3);
	uint32_t colourStream = meshBuilder.AddVertexStream(4);
	uint32_t normalLookupStream = meshBuilder.AddVertexStream(1);

	meshBuilder.BeginChunk();
	for (auto q = extractor.Begin(); q != extractor.End(); ++q)
	{
		const auto& material = materials.GetMaterial(q->m_sourceData);
		const float normal = static_cast<float>(q->m_normal);
		const glm::vec4& colour = material.Colour();

		meshBuilder.BeginTriangle();
		meshBuilder.SetStreamData(posStream, q->m_vertices[0], q->m_vertices[1], q->m_vertices[2]);
		meshBuilder.SetStreamData(colourStream, colour, colour, colour);
		meshBuilder.SetStreamData(normalLookupStream, normal, normal, normal);
		meshBuilder.EndTriangle();

		meshBuilder.BeginTriangle();
		meshBuilder.SetStreamData(posStream, q->m_vertices[0], q->m_vertices[2], q->m_vertices[3]);
		meshBuilder.SetStreamData(colourStream, colour, colour, colour);
		meshBuilder.SetStreamData(normalLookupStream, normal, normal, normal);
		meshBuilder.EndTriangle();
	}
	meshBuilder.EndChunk();
	meshBuilder.CreateMesh(targetMesh);
}