#pragma once

#include "core/system.h"
#include "render/render_pass.h"
#include "sde/debug_camera_controller.h"
#include <memory>

namespace Render
{
	class Material;
	class Mesh;
}

namespace Engine
{
	class InputSystem;
}

namespace SDE
{
	class RenderSystem;
}

class AppSkeleton : public Core::ISystem
{
public:
	AppSkeleton();
	virtual ~AppSkeleton();
	bool PreInit(Core::ISystemEnumerator& systemEnumerator);
	bool PostInit();

	bool Tick();
	void Shutdown();

private:
	void OnEventRecieved(const Core::EngineEvent& e);
	bool m_quit;

	std::shared_ptr<Render::Material> CreateMaterial();
	bool CreateMesh();

	uint32_t m_forwardPassId;
	std::unique_ptr<SDE::DebugCameraController> m_debugCameraController;
	std::shared_ptr<Render::Mesh> m_mesh;
	Engine::InputSystem* m_inputSystem;
	SDE::RenderSystem* m_renderSystem;
};