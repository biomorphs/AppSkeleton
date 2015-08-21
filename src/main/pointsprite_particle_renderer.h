#include "particle_renderer.h"
#include "particle_container.h"
#include "render/material_asset.h"
#include "sde/debug_render.h"

namespace Render
{
	class Mesh;
}

class PointSpriteParticleRenderer : public ParticleRenderer
{
public:
	PointSpriteParticleRenderer() {}
	PointSpriteParticleRenderer(SDE::DebugRender* rnd) : m_debugRender(rnd) {}
	virtual ~PointSpriteParticleRenderer() {}

	void Create(std::shared_ptr<Render::MaterialAsset>& material);
	void Destroy();
	virtual void Render(double deltaTime, const ParticleContainer& container);
	void PushToRenderPass(Render::Camera& camera, Render::RenderPass& targetPass);

private:
	SDE::DebugRender* m_debugRender;
	std::shared_ptr<Render::MaterialAsset> m_material;

	static const uint32_t c_maxPoints = 1024 * 1024 * 1;
	static const uint32_t c_maxBuffers = 2;
	std::unique_ptr<Render::Mesh> m_renderMesh[2];		// double-buffered
	uint32_t m_currentWriteMesh;						// mesh to write to this frame
};