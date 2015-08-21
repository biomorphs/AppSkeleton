#include "particles_stats.h"
#include "debug_gui/debug_gui_system.h"

ParticlesStats::ParticlesStats()
	: m_activeEffectsGraphData(256)
	, m_activeParticlesGraphData(256)
	, m_activeMemoryGraphData(256)
	, m_totalMemoryGraphData(256)
	, m_updateTimeGraphData(256)
{
}

ParticlesStats::~ParticlesStats()
{

}

void ParticlesStats::UpdateStats(uint32_t activeEffects, uint32_t activeParticles, size_t activeMemory, size_t totalMemory, double updateTime)
{
	m_activeEffects = activeEffects;
	m_activeParticles = activeParticles;
	m_activeParticleMemory = activeMemory;
	m_totalParticleMemory = totalMemory;
	m_updateTime = updateTime;

	m_activeEffectsGraphData.PushValue((float)activeEffects);
	m_activeParticlesGraphData.PushValue((float)activeParticles);
	m_activeMemoryGraphData.PushValue((float)activeMemory);
	m_totalMemoryGraphData.PushValue((float)totalMemory);
	m_updateTimeGraphData.PushValue((float)updateTime * 1000.0f);
}

void ParticlesStats::DisplayDebugGui(DebugGui::DebugGuiSystem& gui)
{
	bool alwaysOpen = true;
	char labelBuffer[256] = { '\0' };

	gui.BeginWindow(alwaysOpen, "Particle Stats");
	
	sprintf_s(labelBuffer, "Update time: %2.1f ms", m_updateTime * 1000.0f);
	gui.GraphLines(labelBuffer, glm::vec2(200, 64), m_updateTimeGraphData);

	sprintf_s(labelBuffer, "Active Effects: %d", m_activeEffects);
	gui.GraphLines(labelBuffer, glm::vec2(200, 64), m_activeEffectsGraphData);

	sprintf_s(labelBuffer, "Active Particles: %d", m_activeParticles);
	gui.GraphLines(labelBuffer, glm::vec2(200, 64), m_activeParticlesGraphData);

	sprintf_s(labelBuffer, "Active Memory: %2.1f mb", m_activeParticleMemory / (1024.0f * 1024.0f));
	gui.GraphLines(labelBuffer, glm::vec2(200, 64), m_activeMemoryGraphData);

	sprintf_s(labelBuffer, "Total Memory: %2.1f mb", m_totalParticleMemory / (1024.0f * 1024.0f));
	gui.GraphLines(labelBuffer, glm::vec2(200, 64), m_totalMemoryGraphData);

	gui.EndWindow();
}