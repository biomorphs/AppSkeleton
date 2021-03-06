#include "particle_manager.h"
#include "particle_effect.h"
#include "particles_stats.h"
#include <algorithm>

ParticleManager::ParticleManager()
	: m_effectPool(c_maxEffects)
{
	m_activeEffects.reserve(c_maxEffects);
	m_effectsToKill.reserve(c_maxEffects);
}

ParticleManager::~ParticleManager()
{

}

void ParticleManager::PopulateStats(ParticlesStats& target)
{
	uint32_t activeEffects = (uint32_t)m_activeEffects.size();
	uint32_t activeParticles = 0;
	size_t activeMemory = 0;
	size_t totalMemory = 0;
	for (const auto& it : m_activeEffects)
	{
		activeParticles += it->AliveParticles();
		activeMemory += it->AliveParticles() * it->ParticleSizeBytes();
		totalMemory += it->MaxParticles() * it->ParticleSizeBytes();
	}
	target.UpdateStats(activeEffects, activeParticles, activeMemory, totalMemory, m_lastUpdateTime);
}

ParticleEffect* ParticleManager::AddEffect(uint32_t maxParticles)
{
	ParticleEffect* newEffect = m_effectPool.Allocate();
	if (newEffect)
	{
		newEffect->Create(maxParticles);
		SDE_ASSERT(newEffect != nullptr);
		return newEffect;
	}
	else
	{
		return nullptr;
	}
}

void ParticleManager::StartEffect(ParticleEffect* effect)
{
	SDE_ASSERT(m_effectPool.OwnsPtr(effect));
	m_activeEffects.push_back(effect);
}

bool ParticleManager::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	return true;
}

bool ParticleManager::Initialise()
{
	return true;
}

bool ParticleManager::Tick()
{
	static uint64_t s_lastTime = m_timer.GetTicks();
	uint64_t thisTime = m_timer.GetTicks();
	double elapsedSeconds = (thisTime - s_lastTime) / (double)m_timer.GetFrequency();
	s_lastTime = thisTime;
	elapsedSeconds = std::max(elapsedSeconds, 0.003125);	// Clamp fastest update to 320fps
	elapsedSeconds = std::min(elapsedSeconds, 1.0);			// Clamp slowest update to 1 fps
	auto effectCount = m_activeEffects.size();

	uint64_t startTime = thisTime;
	for (auto i = 0; i < effectCount; ++i)
	{
		auto effect = m_activeEffects[i];
		if (effect->Update(elapsedSeconds) == false)	// This effect should die
		{
			m_effectPool.Free(effect);
			m_effectsToKill.push_back(i);
		}
	}
	uint64_t endTime = m_timer.GetTicks();
	m_lastUpdateTime = (double)(endTime - startTime) / (double)m_timer.GetFrequency();

	for (int32_t i = (int32_t)m_effectsToKill.size() - 1; i >= 0; --i)
	{
		m_activeEffects.erase(m_activeEffects.begin() + m_effectsToKill[i]);
	}
	m_effectsToKill.clear();

	return true;
}

void ParticleManager::Shutdown()
{
	for (const auto& it : m_activeEffects)
	{
		m_effectPool.Free(it);
	}
	m_activeEffects.clear();
	SDE_ASSERT(m_effectPool.ObjectsAllocated() == 0 && m_effectPool.ObjectsFree() == m_effectPool.PoolSize());
}