#pragma once

#include "voxel_definitions.h"
#include "floor.h"
#include "pointsprite_particle_renderer.h"
#include "core/system.h"
#include "sde/debug_camera_controller.h"
#include "render/camera.h"
#include <memory>

namespace Input
{
	class InputSystem;
}

namespace SDE
{
	class RenderSystem;
	class AssetSystem;
	class JobSystem;
	class DebugRender;
}

namespace DebugGui
{
	class DebugGuiSystem;
}

class ParticleManager;

class AppSkeleton : public Core::ISystem
{
public:
	AppSkeleton();
	virtual ~AppSkeleton();
	bool PreInit(Core::ISystemEnumerator& systemEnumerator);
	bool PostInit();

	bool Tick();
	void Shutdown();

	void SpawnParticlesAt(glm::vec3 position);

private:

	static const uint32_t c_windowWidth = 1280;
	static const uint32_t c_windowHeight = 720;
	void InitialiseParticles(std::shared_ptr<Assets::Asset>& materialAsset);
	void InitialiseFloor(std::shared_ptr<Assets::Asset>& materialAsset);

	std::shared_ptr<PointSpriteParticleRenderer> m_pointRender;
	std::unique_ptr<SDE::DebugRender> m_debugRender;
	std::unique_ptr<Floor> m_testFloor;
	std::unique_ptr<SDE::DebugCameraController> m_debugCameraController;
	Render::Camera m_camera;
	SDE::RenderSystem* m_renderSystem;
	SDE::AssetSystem* m_assetSystem;
	Input::InputSystem* m_inputSystem;
	SDE::JobSystem* m_jobSystem;
	DebugGui::DebugGuiSystem* m_debugGui;
	ParticleManager* m_particles;
	uint32_t m_forwardPassId;
	uint32_t m_debugRenderPassId;
};