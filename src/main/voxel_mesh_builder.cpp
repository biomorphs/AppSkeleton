#include "voxel_mesh_builder.h"
#include "voxel_material.h"
#include "vox/greedy_quad_extractor.h"
#include "render/mesh_builder.h"

void GenerateUVs(const Vox::GreedyQuadExtractor<VoxelModel>::QuadDescriptor&q, const VoxelMaterial& mat, glm::vec3(&uv)[4])
{
	// Normal dir is 0-1 -> x, 2-3 -> y, 4-5 -> z
	uint32_t uvAxes[2];
	switch (q.m_normal)
	{
	case Vox::GreedyQuadExtractor<VoxelModel>::QuadDescriptor::NormalDirection::XAxisNegative:
	case Vox::GreedyQuadExtractor<VoxelModel>::QuadDescriptor::NormalDirection::XAxisPositive:
		uvAxes[0] = 2;
		uvAxes[1] = 1;
		break;
	case Vox::GreedyQuadExtractor<VoxelModel>::QuadDescriptor::NormalDirection::YAxisNegative:
	case Vox::GreedyQuadExtractor<VoxelModel>::QuadDescriptor::NormalDirection::YAxisPositive:
		uvAxes[0] = 0;
		uvAxes[1] = 2;
		break;
	case Vox::GreedyQuadExtractor<VoxelModel>::QuadDescriptor::NormalDirection::ZAxisNegative:
	case Vox::GreedyQuadExtractor<VoxelModel>::QuadDescriptor::NormalDirection::ZAxisPositive:
		uvAxes[0] = 0;
		uvAxes[1] = 1;
		break;
	}
	uv[0] = glm::vec3(q.m_vertices[0][uvAxes[0]] * 0.25f, q.m_vertices[0][uvAxes[1]] * 0.25f, mat.TextureIndex());
	uv[1] = glm::vec3(q.m_vertices[1][uvAxes[0]] * 0.25f, q.m_vertices[1][uvAxes[1]] * 0.25f, mat.TextureIndex());
	uv[2] = glm::vec3(q.m_vertices[2][uvAxes[0]] * 0.25f, q.m_vertices[2][uvAxes[1]] * 0.25f, mat.TextureIndex());
	uv[3] = glm::vec3(q.m_vertices[3][uvAxes[0]] * 0.25f, q.m_vertices[3][uvAxes[1]] * 0.25f, mat.TextureIndex());
}

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
	uint32_t uvStream = targetMesh.AddVertexStream(3);
	uint32_t normalLookupStream = targetMesh.AddVertexStream(1);

	glm::vec4 giResults[4];

	targetMesh.BeginChunk();
	for (auto q = extractor.Begin(); q != extractor.End(); ++q)
	{
		const auto& material = materials.GetMaterial(q->m_sourceData);
		const float normal = static_cast<float>(q->m_normal);
		const glm::vec4& colour = material.Colour();
		glm::vec3 uvs[4];
		GenerateUVs(*q, material, uvs);

		targetMesh.BeginTriangle();
		targetMesh.SetStreamData(posStream, q->m_vertices[0], q->m_vertices[1], q->m_vertices[2]);
		targetMesh.SetStreamData(colourStream, colour, colour, colour);
		targetMesh.SetStreamData(uvStream, uvs[0], uvs[1], uvs[2]);
		targetMesh.SetStreamData(normalLookupStream, normal, normal, normal);
		targetMesh.EndTriangle();

		targetMesh.BeginTriangle();
		targetMesh.SetStreamData(posStream, q->m_vertices[0], q->m_vertices[2], q->m_vertices[3]);
		targetMesh.SetStreamData(colourStream, colour, colour, colour);
		targetMesh.SetStreamData(uvStream, uvs[0], uvs[2], uvs[3]);
		targetMesh.SetStreamData(normalLookupStream, normal, normal, normal);
		targetMesh.EndTriangle();
	}
	targetMesh.EndChunk();
}