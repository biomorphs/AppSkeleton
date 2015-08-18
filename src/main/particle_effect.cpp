#include "particle_effect.h"
#include "particle_effect_lifetime.h"
#include "particle_emitter.h"
#include "particle_generator.h"
#include "particle_updater.h"
#include "particle_renderer.h"
#include <algorithm>

ParticleEffect::ParticleEffect(uint32_t maxParticles)
	: m_particles(maxParticles)
{

}

ParticleEffect::ParticleEffect()
{

}

ParticleEffect::~ParticleEffect()
{

}

void ParticleEffect::Create(uint32_t maxParticles)
{
	m_particles.Create(maxParticles);
}

void ParticleEffect::SetLifetime(std::shared_ptr<ParticleEffectLifetime> lifetime)
{
	m_lifetime = lifetime;
}

void ParticleEffect::AddEmitter(std::shared_ptr<ParticleEmitter> emitter)
{
	m_emitters.push_back(emitter);
}

void ParticleEffect::AddGenerator(std::shared_ptr<ParticleGenerator> generator)
{
	m_generators.push_back(generator);
}

void ParticleEffect::AddUpdater(std::shared_ptr<ParticleUpdater> updater)
{
	m_updaters.push_back(updater);
}

void ParticleEffect::AddRenderer(std::shared_ptr<ParticleRenderer> render)
{
	m_renderers.push_back(render);
}

bool ParticleEffect::Update(double deltaTime)
{
	// Emission pass, calculate emission count, generate particles
	uint32_t emissionCount = 0;
	for (const auto& it : m_emitters)
	{
		emissionCount += it->Emit(deltaTime);
	}
	emissionCount = std::min(emissionCount, m_particles.MaxParticles() - m_particles.AliveParticles());
	if (emissionCount > 0)
	{
		const uint32_t startIndex = m_particles.Wake(emissionCount);
		const uint32_t endIndex = m_particles.AliveParticles();
		for (const auto& it : m_generators)
		{
			it->Generate(deltaTime, m_particles, startIndex, endIndex);
		}
	}

	// Update pass - run on all particles
	for (const auto& it : m_updaters)
	{
		it->Update(deltaTime, m_particles);	// Particles may be killed during this
	}

	// Render Pass - run on all particles
	for (const auto& it : m_renderers)
	{
		it->Render(deltaTime, m_particles);
	}

	// Finally, determine if the effect should end
	if (!m_lifetime->ShouldKill(deltaTime, *this))
	{
		return true;
	}
	else
	{
		return false;
	}
}