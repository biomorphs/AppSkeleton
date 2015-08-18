#pragma once

#include "sde/debug_render.h"
#include "particle_container.h"
#include "particle_generator.h"
#include "particle_emitter.h"
#include "particle_updater.h"
#include "particle_renderer.h"
#include "particle_effect_lifetime.h"
#include "particle_effect.h"
#include <glm.hpp>

namespace ParticleEffects
{
	class EmitStaticCount : public ParticleEmitter
	{
	public:
		EmitStaticCount(uint32_t particlesPerUpdate) 
			: m_particlesPerUpdate(particlesPerUpdate) 
		{ 
		}
		virtual ~EmitStaticCount() { }
		virtual uint32_t Emit(double deltaTime)
		{
			return m_particlesPerUpdate;
		}
	private:
		uint32_t m_particlesPerUpdate;
	};

	class EmitBurst : public ParticleEmitter
	{
	public:
		EmitBurst(uint32_t burstCount)
			: m_burstCount(burstCount)
		{
		}
		virtual ~EmitBurst() { }
		virtual uint32_t Emit(double deltaTime)
		{
			uint32_t cnt = m_burstCount;
			m_burstCount = 0;
			return cnt;
		}
	private:
		uint32_t m_burstCount;
	};

	class GenerateStaticPosition : public ParticleGenerator
	{
	public:
		GenerateStaticPosition(const glm::vec3& pos) 
			: m_position(pos)
		{
		}
		virtual ~GenerateStaticPosition() {}
		virtual void Generate(double deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex)
		{
			for (uint32_t i = startIndex; i < endIndex; ++i)
			{
				container.Positions().SetValue(i, m_position);
			}
		}
	private:
		glm::vec3 m_position;
	};

	class GenerateRandomVelocity : public ParticleGenerator
	{
	public:
		GenerateRandomVelocity(const glm::vec3& vMin, const glm::vec3& vMax) 
		: m_vMin(vMin), m_vMax(vMax) {}
		virtual ~GenerateRandomVelocity() {}
		virtual void Generate(double deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex)
		{
			for (uint32_t i = startIndex; i < endIndex; ++i)
			{
				float vX = ((float)rand() / (float)RAND_MAX);
				float vY = ((float)rand() / (float)RAND_MAX);
				float vZ = ((float)rand() / (float)RAND_MAX);
				glm::vec3 range = m_vMax - m_vMin;
				container.Velocities().SetValue(i, glm::vec3(m_vMin.x + (vX * range.x), m_vMin.y + (vY * range.y), m_vMin.z + (vZ * range.z)));
			}
		}
	private:
		glm::vec3 m_vMin;
		glm::vec3 m_vMax;
	};

	class GenerateSimpleLifetime : public ParticleGenerator
	{
	public:
		GenerateSimpleLifetime(float t)
			: m_lifetime(t)
		{
		}
		virtual ~GenerateSimpleLifetime() {}
		virtual void Generate(double deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex)
		{
			for (uint32_t i = startIndex; i < endIndex; ++i)
			{
				container.Lifetimes().SetValue(i, m_lifetime);
			}
		}
	private:
		float m_lifetime;
	};

	class NullUpdater : public ParticleUpdater
	{
	public:
		NullUpdater() {}
		virtual ~NullUpdater() {}
		virtual void Update(double deltaTime, ParticleContainer& container)
		{
		}
	};

	class GravityUpdater : public ParticleUpdater
	{
	public:
		GravityUpdater(float grav) : m_gravity(grav) {}
		virtual ~GravityUpdater() {}
		virtual void Update(double deltaTime, ParticleContainer& container)
		{
			const uint32_t endIndex = container.AliveParticles();
			for (uint32_t i = 0; i < endIndex; ++i)
			{
				glm::vec3 v = container.Velocities().GetValue(i);
				v.y += m_gravity * (float)deltaTime;
				container.Velocities().SetValue(i, v);
			}
		}
	private:
		float m_gravity;
	};

	class EulerPositionUpdater : public ParticleUpdater
	{
	public:
		EulerPositionUpdater() {}
		virtual ~EulerPositionUpdater() {}
		virtual void Update(double deltaTime, ParticleContainer& container)
		{
			const uint32_t endIndex = container.AliveParticles();
			for (uint32_t i = 0; i < endIndex; ++i)
			{
				glm::vec3 p = container.Positions().GetValue(i);
				glm::vec3 v = container.Velocities().GetValue(i);
				p = p + (v * (float)deltaTime);
				container.Positions().SetValue(i, p);
			}
		}
	};

	class KillOnZeroLife : public ParticleUpdater
	{
	public:
		KillOnZeroLife() {}
		virtual ~KillOnZeroLife() {}
		virtual void Update(double deltaTime, ParticleContainer& container)
		{
			uint32_t currentP = 0;
			while(currentP < container.AliveParticles())
			{
				float currentLife = container.Lifetimes().GetValue(currentP);
				currentLife -= (float)deltaTime;
				container.Lifetimes().SetValue(currentP, currentLife);

				if (currentLife <= 0.0f)
				{
					container.Kill(currentP);
				}
				else
				{	
					++currentP;
				}
			}
		}
	};

	class DebugParticleRenderer : public ParticleRenderer
	{
	public:
		DebugParticleRenderer() {}
		DebugParticleRenderer(SDE::DebugRender* rnd) : m_debugRender(rnd) {}
		virtual ~DebugParticleRenderer() {}
		virtual void Render(double deltaTime, const ParticleContainer& container)
		{
			const uint32_t endIndex = container.AliveParticles();
			for (uint32_t i = 0; i < endIndex; ++i)
			{
				m_debugRender->AddAxisAtPoint(container.Positions().GetValue(i), 0.05f);
			}
		}
	private:
		SDE::DebugRender* m_debugRender;
	};

	class LiveForever : public ParticleEffectLifetime
	{
	public:
		LiveForever() {}
		virtual ~LiveForever() {}
		virtual bool ShouldKill(double deltaTime, ParticleEffect& effect)
		{
			return false;
		}
	};

	class KillOnZeroParticles : public ParticleEffectLifetime
	{
	public:
		KillOnZeroParticles() {}
		virtual ~KillOnZeroParticles() {}
		virtual bool ShouldKill(double deltaTime, ParticleEffect& effect)
		{
			return effect.AliveParticles() == 0;
		}
	};
}