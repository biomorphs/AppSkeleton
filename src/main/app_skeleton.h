#pragma once

#include "voxel_definitions.h"
#include "floor.h"
#include "core/system.h"
#include "sde/debug_camera_controller.h"
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
	void InitialiseFloor(std::shared_ptr<Assets::Asset>& materialAsset);

	std::unique_ptr<Floor> m_testFloor;
	std::unique_ptr<SDE::DebugCameraController> m_debugCameraController;
	SDE::RenderSystem* m_renderSystem;
	SDE::AssetSystem* m_assetSystem;
	Input::InputSystem* m_inputSystem;
	SDE::JobSystem* m_jobSystem;
	uint32_t m_forwardPassId;
};