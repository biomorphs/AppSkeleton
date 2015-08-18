#include "particle_tests.h"
#include "particle_effect_lifetime.h"
#include "particle_generator.h"
#include "particle_emitter.h"
#include "particle_updater.h"
#include "particle_renderer.h"
#include "particle_effect.h"
#include "particle_effects.h"

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

	class NullRender : public ParticleRenderer
	{
	public:
		NullRender() {}
		virtual ~NullRender() {}
		virtual void Render(double deltaTime, const ParticleContainer& container)
		{
		}
	};

	class TestPositionUpdater : public ParticleUpdater
	{
	public:
		TestPositionUpdater(const glm::vec3& pos) 
			: m_position(pos)
		{
		}
		virtual ~TestPositionUpdater() {}
		virtual void Update(double deltaTime, ParticleContainer& container)
		{
			for (uint32_t i = 0; i < container.AliveParticles(); ++i)
			{
				SDE_ASSERT(container.Positions().GetValue(i) == m_position);
			}
		}
	private:
		glm::vec3 m_position;
	};

	void NullTest()
	{
		ParticleEffect testSystem(1024);
		testSystem.AddEmitter(std::shared_ptr<ParticleEmitter>(new NullEmitter()));
		testSystem.AddGenerator(std::shared_ptr<ParticleGenerator>(new NullGenerator()));
		testSystem.AddUpdater(std::shared_ptr<ParticleUpdater>(new ParticleEffects::NullUpdater()));
		testSystem.AddRenderer(std::shared_ptr<ParticleRenderer>(new NullRender()));
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
		testSystem.AddRenderer(std::shared_ptr<ParticleRenderer>(new NullRender()));
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