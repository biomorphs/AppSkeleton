#include "render_system.h"
#include "render/window.h"
#include "render/device.h"
#include "render/sprite_layer.h"
#include "kernel/assert.h"
#include "kernel/refcounted_data.h"
#include "kernel/handle.h"
#include "kernel/object_bucket.h"

RenderSystem::RenderSystem()
: m_quit(false)
, m_window(nullptr)
, m_device(nullptr)
, m_textureCache(nullptr)
{
}

bool RenderSystem::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	m_window = new Render::Window(Render::Window::Properties("Skeleton Application", 640, 480));
	m_device = new Render::Device(*m_window);
	m_textureCache = new Render::TextureCache(m_device);
	m_spriteManager = new Render::SpriteManager();
	m_spriteRender = new Render::SpriteRender(m_device, 8);
	m_window->Show();

	Render::TextureHandle texture = m_textureCache->GetTexture("rpgtiles.png");
	m_testSprite = m_spriteManager->AddSpriteDefinition(texture, Math::Rect(0, 32, 32, 32));
	m_testSprite2 = m_spriteManager->AddSpriteDefinition(texture, Math::Rect(480, 32, 32, 32));

	m_spriteRender->AddLayer(4096);
	m_spriteRender->AddLayer(1024);

	return true;
}

void RenderSystem::OnEventRecieved(const Core::EngineEvent& e)
{
	if (e.m_type == Core::EngineEvent::QuitRequest)
	{
		m_quit = true;
	}
}

bool RenderSystem::Tick()
{
	Render::SpriteLayer* theLayer = m_spriteRender->GetLayer(0);
	for (int32_t x = 0; x < 640; x += 64)
	{
		for (int32_t y = 0; y < 480; y += 64)
		{
			theLayer->PushInstance(m_testSprite, Math::Vector2(x, y), Math::Vector2(64, 64));
		}
	}

	srand(120);
	theLayer = m_spriteRender->GetLayer(1);
	for (int32_t x = 0; x < 640; x += 64)
	{
		for (int32_t y = 0; y < 480; y += 64)
		{
			if (rand() % 100 < 20)
			{
				theLayer->PushInstance(m_testSprite2, Math::Vector2(x, y), Math::Vector2(64, 64));
			}
		}
	}

	m_spriteRender->DrawLayers();
	m_device->Present();
	return !m_quit;
}

void RenderSystem::Shutdown()
{	
	// get rid of the sprite handles
	m_testSprite = Render::SpriteDefHandle();
	m_testSprite2 = Render::SpriteDefHandle();

	m_window->Hide();
	delete m_spriteRender;
	delete m_spriteManager;
	delete m_textureCache;
	delete m_device;
	delete m_window;
}