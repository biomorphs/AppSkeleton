#pragma once

#include "sde/debug_render.h"
#include "particle_container.h"
#include "particle_generator.h"
#include "particle_emitter.h"
#include "particle_updater.h"
#include "particle_renderer.h"
#include "particle_effect_lifetime.h"
#include "particle_effect.h"
#include <glm/glm.hpp>

namespace ParticleEffects
{
	class NullRender : public ParticleRenderer
	{
	public:
		NullRender() {}
		virtual ~NullRender() {}
		virtual void Render(double deltaTime, const ParticleContainer& container);
	};

	class EmitStaticCount : public ParticleEmitter
	{
	public:
		EmitStaticCount(uint32_t particlesPerUpdate) 
			: m_particlesPerUpdate(particlesPerUpdate) 
		{ 
		}
		virtual ~EmitStaticCount() { }
		virtual uint32_t Emit(double deltaTime);
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
		virtual uint32_t Emit(double deltaTime);
	private:
		uint32_t m_burstCount;
	};

	class GenerateStaticPosition : public ParticleGenerator
	{
	public:
		GenerateStaticPosition(const glm::vec3& pos) 
			: m_position(pos, 0.0f)
		{
		}
		virtual ~GenerateStaticPosition() {}
		virtual void Generate(double deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex);
	private:
		__declspec(align(16)) glm::vec4 m_position;
	};

	class GenerateRandomVelocity : public ParticleGenerator
	{
	public:
		GenerateRandomVelocity(const glm::vec3& vMin, const glm::vec3& vMax) 
		: m_vMin(vMin,0.0f), m_vMax(vMax,0.0f) {}
		virtual ~GenerateRandomVelocity() {}
		virtual void Generate(double deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex);
	private:
		glm::vec4 m_vMin;
		glm::vec4 m_vMax;
	};

	class GenerateRandomLifetime : public ParticleGenerator
	{
	public:
		GenerateRandomLifetime(float tMin, float tMax)
			: m_timeMin(tMin)
			, m_timeMax(tMax)
		{
		}
		virtual ~GenerateRandomLifetime() {}
		virtual void Generate(double deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex);
	private:
		float m_timeMin;
		float m_timeMax;
	};

	class GenerateSimpleLifetime : public ParticleGenerator
	{
	public:
		GenerateSimpleLifetime(float t)
			: m_lifetime(t)
		{
		}
		virtual ~GenerateSimpleLifetime() {}
		virtual void Generate(double deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex);
	private:
		float m_lifetime;
	};

	class NullUpdater : public ParticleUpdater
	{
	public:
		NullUpdater() {}
		virtual ~NullUpdater() {}
		virtual void Update(double deltaTime, ParticleContainer& container);
	};

	class GravityUpdater : public ParticleUpdater
	{
	public:
		GravityUpdater(float grav) : m_gravity(0.0f,grav,0.0f,0.0f) {}
		virtual ~GravityUpdater() {}
		virtual void Update(double deltaTime, ParticleContainer& container);
	private:
		__declspec(align(16)) glm::vec4 m_gravity;
	};

	class EulerPositionUpdater : public ParticleUpdater
	{
	public:
		EulerPositionUpdater() {}
		virtual ~EulerPositionUpdater() {}
		virtual void Update(double deltaTime, ParticleContainer& container);
	};

	class EulerFloorBouncer : public ParticleUpdater
	{
	public:
		EulerFloorBouncer(const float floorHeight) : m_floorHeight(floorHeight) {}
		virtual ~EulerFloorBouncer() {}
		virtual void Update(double deltaTime, ParticleContainer& container);
	private:
		float m_floorHeight;
	};

	class ColourFader : public ParticleUpdater
	{
	public:
		ColourFader(const glm::vec4& c0, const glm::vec4& c1, float ltStart, float ltEnd) 
			: m_c0(c0), m_c1(c1), m_lifetimeStart(ltStart), m_lifetimeEnd(ltEnd) {}
		virtual ~ColourFader() {}
		virtual void Update(double deltaTime, ParticleContainer& container);
	private:
		__declspec(align(16)) glm::vec4 m_c0;
		__declspec(align(16)) glm::vec4 m_c1;
		float m_lifetimeStart;
		float m_lifetimeEnd;
	};

	class KillOnZeroLife : public ParticleUpdater
	{
	public:
		KillOnZeroLife() {}
		virtual ~KillOnZeroLife() {}
		virtual void Update(double deltaTime, ParticleContainer& container);
	};

	class DebugParticleRenderer : public ParticleRenderer
	{
	public:
		DebugParticleRenderer() {}
		DebugParticleRenderer(SDE::DebugRender* rnd) : m_debugRender(rnd) {}
		virtual ~DebugParticleRenderer() {}
		virtual void Render(double deltaTime, const ParticleContainer& container);
	private:
		SDE::DebugRender* m_debugRender;
	};

	class LiveForever : public ParticleEffectLifetime
	{
	public:
		LiveForever() {}
		virtual ~LiveForever() {}
		virtual bool ShouldKill(double deltaTime, ParticleEffect& effect);
	};

	class KillOnZeroParticles : public ParticleEffectLifetime
	{
	public:
		KillOnZeroParticles() {}
		virtual ~KillOnZeroParticles() {}
		virtual bool ShouldKill(double deltaTime, ParticleEffect& effect);
	};
}