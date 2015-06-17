#pragma once

#include "core/system.h"
#include "core/system_enumerator.h"
#include "core/event_listener.h"
#include "render/window.h"
#include "render/device.h"
#include "render/shader_binary.h"
#include "render/shader_program.h"
#include "render/mesh.h"
#include "render/material.h"
#include <memory>

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
	std::shared_ptr<Render::Mesh> m_mesh;
};