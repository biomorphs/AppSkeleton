#pragma once

#include "core/system.h"
#include "render/render_pass.h"
#include "sde/debug_camera_controller.h"
#include <memory>

namespace Render
{
	class Window;
	class Device;
	class Material;
	class Mesh;
}

namespace Engine
{
	class InputSystem;
}

class RenderSystem : public Core::ISystem
{
public:
	RenderSystem();
	virtual ~RenderSystem();
	bool PreInit(Core::ISystemEnumerator& systemEnumerator);
	bool Tick();
	void Shutdown();

private:
	void OnEventRecieved(const Core::EngineEvent& e);
	bool m_quit;

	std::shared_ptr<Render::Material> CreateMaterial();
	bool CreateMesh();

	Render::Window* m_window;
	Render::Device* m_device;
	Render::RenderPass m_forwardPass;
	std::shared_ptr<Render::Mesh> m_mesh;
	Engine::InputSystem* m_inputSystem;
	std::unique_ptr<SDE::DebugCameraController> m_debugCameraController;
	
};