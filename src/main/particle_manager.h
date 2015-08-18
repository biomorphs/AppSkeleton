#pragma once

#include "kernel/base_types.h"
#include "core/system.h"
#include "core/object_pool.h"
#include "core/timer.h"
#include <vector>
#include <memory>

class ParticleEffect;
class ParticleEmitter;
class ParticleGenerator;
class ParticleUpdater;
class ParticleRenderer;
class ParticleEffectLifetime;

class ParticleManager : public Core::ISystem
{
public:
	ParticleManager();
	virtual ~ParticleManager();

	ParticleEffect* AddEffect(uint32_t maxParticles);
	void StartEffect(ParticleEffect* effect);

	virtual bool PreInit(Core::ISystemEnumerator& systemEnumerator);
	virtual bool Initialise();
	virtual bool Tick();
	virtual void Shutdown();

private:
	static const uint32_t c_maxEffects = 1024;
	Core::ObjectPool< ParticleEffect > m_effectPool;
	std::vector<ParticleEffect*> m_activeEffects;
	std::vector<uint32_t> m_effectsToKill;
	Core::Timer m_timer;
};