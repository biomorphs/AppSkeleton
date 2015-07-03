#pragma once

#include "core/system.h"
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
	class AssetSystem;
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
	bool CreateMesh();
	void InitialiseVoxelModel();
	void DebugRenderVoxelModel();

	uint32_t m_forwardPassId;
	std::unique_ptr<SDE::DebugCameraController> m_debugCameraController;
	std::unique_ptr<Render::Mesh> m_mesh;
	SDE::RenderSystem* m_renderSystem;
	SDE::AssetSystem* m_assetSystem;
	Engine::InputSystem* m_inputSystem;
};