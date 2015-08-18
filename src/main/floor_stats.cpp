#include "floor_stats.h"
#include "debug_gui/debug_gui_system.h"

FloorStats::FloorStats()
	: m_writesPending(0)
	, m_totalVertexBufferBytes(0)
	, m_totalVoxelDataBytes(0)
{
}

FloorStats::~FloorStats()
{
}

void FloorStats::UpdateStats(const Math::Box3& bnds, const glm::vec3& secSize, int32_t wPending, size_t vbBytes, size_t vxBytes)
{
	m_bounds = bnds;
	m_sectionSize = secSize;
	m_writesPending = wPending;
	m_totalVertexBufferBytes = vbBytes;
	m_totalVoxelDataBytes = vxBytes;
}

void FloorStats::showMemStat(DebugGui::DebugGuiSystem& gui, const char* txt, size_t val)
{
	char statsTxt[128] = { '\0' };
	if(val <= 4096)
	{
		sprintf_s(statsTxt, "%s: %lld bytes", txt, val);
	}
	else if (val <= 1024 * 1024)
	{
		sprintf_s(statsTxt, "%s: %.2f kb", txt, val / 1024.0f);
	}
	else if (val <= 1024 * 1024 * 1024)
	{
		sprintf_s(statsTxt, "%s: %.2f mb", txt, val / (float)(1024 * 1024));
	}
	else
	{
		sprintf_s(statsTxt, "%s: %.6f gb", txt, val / (float)(1024 * 1024 * 1024));
	}
	
	gui.Text(statsTxt);
}

void FloorStats::DisplayDebugGui(DebugGui::DebugGuiSystem& gui)
{
	gui.BeginWindow(m_windowOpen, "Floor Stats");
	char statsTxt[512] = { '\0' };
	
	sprintf_s(statsTxt, "Bounds (m): {%2.1f, %2.1f, %2.1f} - {%2.1f, %2.1f, %2.1f}",
		m_bounds.Min().x, m_bounds.Min().y, m_bounds.Min().z,
		m_bounds.Max().x, m_bounds.Max().y, m_bounds.Max().z);
	gui.Text(statsTxt);

	sprintf_s(statsTxt, "Section Dimensions (m): {%2.1f, %2.1f, %2.1f}", m_sectionSize.x, m_sectionSize.y, m_sectionSize.z);
	gui.Text(statsTxt);

	sprintf_s(statsTxt, "Write jobs pending: %d", m_writesPending);
	gui.Text(statsTxt);

	showMemStat(gui, "Vertex Buffer Memory", m_totalVertexBufferBytes);
	showMemStat(gui, "Voxel Data Memory", m_totalVoxelDataBytes);

	gui.EndWindow();
}