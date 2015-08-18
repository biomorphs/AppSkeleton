#pragma once

#include "kernel/base_types.h"

class ParticleContainer;

// Generator is responsible for initialising particles at emission time.
// Generally they should be split with different ones for position, colour, etc
class ParticleGenerator
{
public:
	virtual ~ParticleGenerator() {}
	virtual void Generate(double deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex) = 0;
};