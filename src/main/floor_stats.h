#pragma once

#include "math/box3.h"
#include "kernel/base_types.h"

namespace DebugGui
{
	class DebugGuiSystem;
}

class FloorStats
{
public:
	FloorStats();
	~FloorStats();

	void UpdateStats(const Math::Box3& bnds, const glm::vec3& secSize, int32_t wPending, size_t vbBytes, size_t vxBytes);
	void DisplayDebugGui(DebugGui::DebugGuiSystem& gui);

private:
	void showMemStat(DebugGui::DebugGuiSystem& gui, const char* txt, size_t val);
	Math::Box3 m_bounds;
	glm::vec3 m_sectionSize;
	int32_t m_writesPending;
	size_t m_totalVertexBufferBytes;
	size_t m_totalVoxelDataBytes;
	bool m_windowOpen;
};