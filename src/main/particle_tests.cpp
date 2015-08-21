#include "particle_tests.h"
#include "particle_effect_lifetime.h"
#include "particle_generator.h"
#include "particle_emitter.h"
#include "particle_updater.h"
#include "particle_renderer.h"
#include "particle_effect.h"
#include "particle_effects.h"
#include <glm/gtc/type_ptr.hpp>

namespace ParticleTests
{
	class NullEmitter : public ParticleEmitter
	{
	public:
		NullEmitter() { }
		virtual ~NullEmitter() { }
		virtual uint32_t Emit(double deltaTime)
		{
			return 0;
		}
	};

	class NullGenerator : public ParticleGenerator
	{
	public:
		NullGenerator() {}
		virtual ~NullGenerator() {}
		virtual void Generate(double deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex)
		{
		}
	};

	class TestPositionUpdater : public ParticleUpdater
	{
	public:
		TestPositionUpdater(const glm::vec3& pos) 
			: m_position(pos, 0.0f)
		{
		}
		virtual ~TestPositionUpdater() {}
		virtual void Update(double deltaTime, ParticleContainer& container)
		{
			__declspec(align(16)) glm::vec4 testPosition;
			for (uint32_t i = 0; i < container.AliveParticles(); ++i)
			{
				_mm_store_ps(glm::value_ptr(testPosition), container.Positions().GetValue(i));
				SDE_ASSERT(testPosition == m_position);
			}
		}
	private:
		glm::vec4 m_position;
	};

	void NullTest()
	{
		ParticleEffect testSystem(1024);
		testSystem.AddEmitter(std::shared_ptr<ParticleEmitter>(new NullEmitter()));
		testSystem.AddGenerator(std::shared_ptr<ParticleGenerator>(new NullGenerator()));
		testSystem.AddUpdater(std::shared_ptr<ParticleUpdater>(new ParticleEffects::NullUpdater()));
		testSystem.AddRenderer(std::shared_ptr<ParticleRenderer>(new ParticleEffects::NullRender()));
		testSystem.SetLifetime(std::shared_ptr<ParticleEffectLifetime>(new ParticleEffects::LiveForever()));
		for (int32_t i = 0;i < 10;++i)
		{
			testSystem.Update(0.1f);
		}
	}

	void StaticTest()
	{
		ParticleEffect testSystem(1024);
		testSystem.AddEmitter(std::shared_ptr<ParticleEmitter>(new ParticleEffects::EmitStaticCount(10)));
		testSystem.AddGenerator(std::shared_ptr<ParticleGenerator>(new ParticleEffects::GenerateStaticPosition(glm::vec3(0.0f))));
		testSystem.AddUpdater(std::shared_ptr<ParticleUpdater>(new TestPositionUpdater(glm::vec3(0.0f))));
		testSystem.AddRenderer(std::shared_ptr<ParticleRenderer>(new ParticleEffects::NullRender()));
		testSystem.SetLifetime(std::shared_ptr<ParticleEffectLifetime>(new ParticleEffects::LiveForever()));
		for (int32_t i = 0;i < 10;++i)
		{
			testSystem.Update(0.1f);
		}
		SDE_ASSERT(testSystem.AliveParticles() == 100);
	}

	void RunTests()
	{
		NullTest();
		StaticTest();
	}
}