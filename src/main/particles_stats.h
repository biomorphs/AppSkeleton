#pragma once

#include "math/box3.h"
#include "kernel/base_types.h"
#include "debug_gui/graph_data_buffer.h"

namespace DebugGui
{
	class DebugGuiSystem;
}

class ParticlesStats
{
public:
	ParticlesStats();
	~ParticlesStats();

	void UpdateStats(uint32_t activeEffects, uint32_t activeParticles, size_t activeMemory, size_t totalMemory, double updateTime);
	void DisplayDebugGui(DebugGui::DebugGuiSystem& gui);

private:
	uint32_t m_activeEffects;
	uint32_t m_activeParticles;
	size_t m_activeParticleMemory;
	size_t m_totalParticleMemory;
	double m_updateTime;
	DebugGui::GraphDataBuffer m_activeEffectsGraphData;
	DebugGui::GraphDataBuffer m_activeParticlesGraphData;
	DebugGui::GraphDataBuffer m_activeMemoryGraphData;
	DebugGui::GraphDataBuffer m_totalMemoryGraphData;
	DebugGui::GraphDataBuffer m_updateTimeGraphData;
};