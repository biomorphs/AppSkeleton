#pragma once

#include "kernel/base_types.h"

// ParticleEmitter simply decides when particles should be spawned
class ParticleEmitter
{
public:
	virtual ~ParticleEmitter() {}
	virtual uint32_t Emit(double deltaTime) = 0;	// each emitter can split out a number of particles
};