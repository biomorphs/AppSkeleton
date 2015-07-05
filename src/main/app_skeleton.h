#pragma once

#include "voxel_definitions.h"
#include "floor.h"
#include "core/system.h"
#include "sde/debug_camera_controller.h"
#include <memory>

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
	void InitialiseFloor(std::shared_ptr<Core::Asset>& materialAsset);

	std::unique_ptr<Floor> m_testFloor;
	std::unique_ptr<SDE::DebugCameraController> m_debugCameraController;
	SDE::RenderSystem* m_renderSystem;
	SDE::AssetSystem* m_assetSystem;
	Engine::InputSystem* m_inputSystem;
	uint32_t m_forwardPassId;
	bool m_quit;
};