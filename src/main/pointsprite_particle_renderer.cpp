#include "pointsprite_particle_renderer.h"
#include "render/camera.h"
#include "render/render_pass.h"
#include "render/mesh.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

void PointSpriteParticleRenderer::PushToRenderPass(Render::Camera& camera, Render::RenderPass& targetPass)
{
	auto& target = *m_renderMesh[m_currentWriteMesh].get();
	auto& theChunk = target.GetChunks()[0];
	auto& posStream = target.GetStreams()[0];
	auto& colourStream = target.GetStreams()[1];

	// push data to gpu
	posStream.SetData(0, m_writeBufferSize * sizeof(glm::vec4), m_posWriteBuffer.get());
	colourStream.SetData(0, m_writeBufferSize * sizeof(glm::vec4), m_colWriteBuffer.get());

	// update render chunk
	theChunk.m_firstVertex = 0;
	theChunk.m_vertexCount = m_writeBufferSize;

	const glm::mat4 modelView = camera.ViewMatrix();	// no model transform
	const glm::mat4 projMat = camera.ProjectionMatrix();
	const glm::vec4 screenSizeSpriteSize(1280.0f, 720.0f, 0.1f, 0.0f);
	Render::UniformBuffer instanceUniforms;
	instanceUniforms.SetValue("modelview", modelView);
	instanceUniforms.SetValue("projection", projMat);
	instanceUniforms.SetValue("screenSize_spriteSize", screenSizeSpriteSize);
	targetPass.AddInstance(m_renderMesh[m_currentWriteMesh].get(), std::move(instanceUniforms));

	// flip buffers
	m_currentWriteMesh = (m_currentWriteMesh + 1) % c_maxBuffers;
	m_writeBufferSize = 0;
}

void PointSpriteParticleRenderer::Create(std::shared_ptr<Assets::Asset>& material)
{
	m_material = material;
	for (uint32_t i = 0; i < c_maxBuffers; ++i)
	{
		auto& mesh = m_renderMesh[i];
		mesh = std::make_unique<Render::Mesh>();
		auto& streams = mesh->GetStreams();
		auto& vertexArray = mesh->GetVertexArray();
		auto& chunks = mesh->GetChunks();

		streams.resize(2);
		Render::RenderBuffer &posBuffer = streams[0], &colBuffer = streams[1];
		if (!posBuffer.Create(c_maxPoints * sizeof(glm::vec4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic))
		{
			SDE_LOGC(SDE, "Failed to create particle pos buffer");
			return;
		}
		if (!colBuffer.Create(c_maxPoints * sizeof(glm::vec4), Render::RenderBufferType::VertexData, Render::RenderBufferModification::Dynamic))
		{
			SDE_LOGC(SDE, "Failed to create particle pos buffer");
			return;
		}

		vertexArray.AddBuffer(0, &streams[0], Render::VertexDataType::Float, 4);
		vertexArray.AddBuffer(1, &streams[1], Render::VertexDataType::Float, 4);
		if (!vertexArray.Create())
		{
			SDE_LOGC(SDE, "Failed to create debug vertex array");
			return;
		}

		// add a chunk for each primitive type
		chunks.push_back(Render::MeshChunk(0, 0, Render::PrimitiveType::PointSprites));
		Render::MaterialAsset* matAsset = (Render::MaterialAsset*)m_material.get();
		mesh->SetMaterial(matAsset->GetMaterial());

		// create the aligned write buffers
		auto deleter = [](glm::vec4* p)
		{
			_aligned_free(p);
		};

		glm::vec4* rawBuffer = (glm::vec4*)_aligned_malloc(c_maxPoints * sizeof(glm::vec4), 16);
		SDE_ASSERT(rawBuffer);
		m_posWriteBuffer = std::unique_ptr<glm::vec4, decltype(deleter)>(rawBuffer, deleter);

		rawBuffer = (glm::vec4*)_aligned_malloc(c_maxPoints * sizeof(glm::vec4), 16);
		SDE_ASSERT(rawBuffer);
		m_colWriteBuffer = std::unique_ptr<glm::vec4, decltype(deleter)>(rawBuffer, deleter);

		m_writeBufferSize = 0;
		m_currentWriteMesh = 0;
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
	const uint32_t particleCount = container.AliveParticles();
	const uint32_t particlesToWrite = std::min(m_writeBufferSize + particleCount, c_maxPoints) - m_writeBufferSize;
	if (particlesToWrite > 0)
	{
		memcpy((void*)(m_posWriteBuffer.get() + m_writeBufferSize), &container.Positions().GetValue(0), particlesToWrite * sizeof(glm::vec4));
		memcpy((void*)(m_colWriteBuffer.get() + m_writeBufferSize), &container.Colours().GetValue(0), particlesToWrite * sizeof(glm::vec4));
		m_writeBufferSize += particlesToWrite;
	}
}