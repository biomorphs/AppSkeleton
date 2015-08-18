#pragma once

class ParticleUpdater
{
public:
	virtual ~ParticleUpdater() {}
	virtual void Update(double deltaTime, ParticleContainer& container) = 0;
};