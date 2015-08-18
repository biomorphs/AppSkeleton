#pragma once

class ParticleEffect;

// Determines when an effect should be killed
class ParticleEffectLifetime
{
public:
	virtual ~ParticleEffectLifetime() {}
	virtual bool ShouldKill(double deltaTime, ParticleEffect& effect) = 0;
};