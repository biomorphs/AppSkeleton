#pragma once

#include "particle_container.h"
#include <vector>

class ParticleEmitter;
class ParticleGenerator;
class ParticleUpdater;
class ParticleRenderer;
class ParticleEffectLifetime;

// A particle effect consists of multiple emitters, each with separate
// generators which initialise each particle on emission.
// Multiple updaters are used, with each having the ability to kill particles
// A ParticleEffectLifetime object determines when the effect should be killed
class ParticleEffect
{
public:
	ParticleEffect();
	ParticleEffect(uint32_t maxParticles);
	~ParticleEffect();

	void Create(uint32_t maxParticles);
	void SetLifetime(std::shared_ptr<ParticleEffectLifetime> lifetime);
	void AddEmitter(std::shared_ptr<ParticleEmitter> emitter);
	void AddGenerator(std::shared_ptr<ParticleGenerator> generator);
	void AddUpdater(std::shared_ptr<ParticleUpdater> updater);
	void AddRenderer(std::shared_ptr<ParticleRenderer> render);
	inline uint32_t AliveParticles() const { return m_particles.AliveParticles(); }
	inline uint32_t MaxParticles() const { return m_particles.MaxParticles(); }
	inline size_t ParticleSizeBytes() const { return m_particles.ParticleSizeBytes(); }

	bool Update(double deltaTime);
private:
	ParticleContainer m_particles;
	std::shared_ptr<ParticleEffectLifetime> m_lifetime;
	std::vector< std::shared_ptr<ParticleEmitter> > m_emitters;
	std::vector< std::shared_ptr<ParticleGenerator> > m_generators;
	std::vector< std::shared_ptr<ParticleUpdater> > m_updaters;
	std::vector< std::shared_ptr<ParticleRenderer> > m_renderers;
};