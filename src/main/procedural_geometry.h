#pragma once

#include <glm.hpp>

inline float Sphere(const glm::vec3& voxelPosition, const glm::vec3& center, float radius)
{
	return radius - glm::distance(voxelPosition, center);
}

inline float Box(const glm::vec3& voxelPosition, const glm::vec3& boxMin, const glm::vec3& boxMax)
{
	const glm::vec3 bpos = (boxMax + boxMin) * 0.5f;
	const glm::vec3 b = (boxMax - boxMin) * 0.5f;
	return glm::length(glm::max(glm::abs(bpos - voxelPosition) - b, glm::vec3(0.0)));
}