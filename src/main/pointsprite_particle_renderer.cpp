#include "pointsprite_particle_renderer.h"
#include "render/mesh.h"
#include <glm/gtc/type_ptr.hpp>

void PointSpriteParticleRenderer::PushToRenderPass(Render::Camera& camera, Render::RenderPass& targetPass)
{

}

void PointSpriteParticleRenderer::Create(std::shared_ptr<Render::MaterialAsset>& material)
{
	for (uint32_t i = 0; i < c_maxBuffers; ++i)
	{
		auto& mesh = m_renderMesh[i];
		mesh = std::make_unique<Render::Mesh>();
		auto& streams = mesh->GetStreams();
		auto& vertexArray = mesh->GetVertexArray();
		auto& chunks = mesh->GetChunks();

		streams.resize(1);
		Render::RenderBuffer &posBuffer = streams[0];
		if (!posBuffer.Create(c_maxPoints * sizeof(glm::vec4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic))
		{
			SDE_LOGC(SDE, "Failed to create particle pos buffer");
			return;
		}
		vertexArray.AddBuffer(0, &streams[0], Render::VertexDataType::Float, 4);
		if (!vertexArray.Create())
		{
			SDE_LOGC(SDE, "Failed to create debug vertex array");
			return;
		}

		// add a chunk for each primitive type
		chunks.push_back(Render::MeshChunk(0, 0, Render::PrimitiveType::PointSprites));
		mesh->SetMaterial(m_material->GetMaterial());
	}
}

void PointSpriteParticleRenderer::Destroy()
{
	for (uint32_t i = 0; i < c_maxBuffers; ++i)
	{
		m_renderMesh[i] = nullptr;
	}
	m_material = nullptr;
}

void PointSpriteParticleRenderer::Render(double deltaTime, const ParticleContainer& container)
{
	const uint32_t endIndex = container.AliveParticles();
	__declspec(align(16)) glm::vec4 positionVec;
	for (uint32_t i = 0; i < endIndex; ++i)
	{
		_mm_store_ps(glm::value_ptr(positionVec), container.Positions().GetValue(i));
		m_debugRender->AddAxisAtPoint(positionVec, 0.05f);
	}
}