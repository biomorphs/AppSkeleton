#pragma once

#include "kernel/base_types.h"
#include "particle_buffer.h"
#include <glm.hpp>

class ParticleContainer
{
public:
	ParticleContainer();
	ParticleContainer(uint32_t maxParticles);
	~ParticleContainer();

	void Create(uint32_t maxParticles);

	inline const ParticleBuffer<glm::vec3>& Positions() const { return m_position; }
	inline ParticleBuffer<glm::vec3>& Positions() { return m_position; }
	inline const ParticleBuffer<glm::vec3>& Velocities() const { return m_velocity; }
	inline ParticleBuffer<glm::vec3>& Velocities() { return m_velocity; }
	inline const ParticleBuffer<float>& Lifetimes() const { return m_lifetime; }
	inline ParticleBuffer<float>& Lifetimes() { return m_lifetime; }

	uint32_t Wake(uint32_t count);
	void Kill(uint32_t index);
	inline uint32_t MaxParticles() const { return m_maxParticles; }
	inline uint32_t AliveParticles() const { return m_livingParticles; }

private:
	uint32_t m_maxParticles;
	uint32_t m_livingParticles;

	ParticleBuffer<glm::vec3> m_position;
	ParticleBuffer<glm::vec3> m_velocity;
	ParticleBuffer<float> m_lifetime;
};

#include "particle_container.inl"