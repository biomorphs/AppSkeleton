#pragma once

class ParticleContainer;

class ParticleRenderer
{
public:
	virtual ~ParticleRenderer() {}
	virtual void Render(double deltaTime, const ParticleContainer& container) = 0;
};