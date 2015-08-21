#include "particle_effects.h"
#include <glm/gtc/type_ptr.hpp>

namespace ParticleEffects
{
	void NullRender::Render(double deltaTime, const ParticleContainer& container)
	{
	}

	uint32_t EmitStaticCount::Emit(double deltaTime)
	{
		return m_particlesPerUpdate;
	}

	uint32_t EmitBurst::Emit(double deltaTime)
	{
		uint32_t cnt = m_burstCount;
		m_burstCount = 0;
		return cnt;
	}

	void GenerateStaticPosition::Generate(double deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex)
	{
		__m128 posVec = _mm_load_ps(glm::value_ptr(m_position));
		for (uint32_t i = startIndex; i < endIndex; ++i)
		{
			_mm_stream_ps((float*)&container.Positions().GetValue(i), posVec);
		}
		_mm_sfence();
	}

	void GenerateRandomVelocity::Generate(double deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex)
	{
		glm::vec4 range = m_vMax - m_vMin;
		for (uint32_t i = startIndex; i < endIndex; ++i)
		{
			float vX = ((float)rand() / (float)RAND_MAX);
			float vY = ((float)rand() / (float)RAND_MAX);
			float vZ = ((float)rand() / (float)RAND_MAX);
			
			__declspec(align(16)) glm::vec4 velocity(m_vMin.x + (vX * range.x), m_vMin.y + (vY * range.y), m_vMin.z + (vZ * range.z), 0.0f);
			__m128 velVec = _mm_load_ps(glm::value_ptr(velocity));
			_mm_stream_ps((float*)&container.Velocities().GetValue(i), velVec);
		}
		_mm_sfence();
	}

	void GenerateSimpleLifetime::Generate(double deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex)
	{
		for (uint32_t i = startIndex; i < endIndex; ++i)
		{
			container.Lifetimes().SetValue(i, m_lifetime);
		}
	}

	void NullUpdater::Update(double deltaTime, ParticleContainer& container)
	{
	}

	void GravityUpdater::Update(double deltaTime, ParticleContainer& container)
	{
		__declspec(align(16)) const glm::vec4 c_deltaTime((float)deltaTime);
		const __m128 c_gravity = _mm_load_ps(glm::value_ptr(m_gravity));
		const __m128 c_deltaVec = _mm_load_ps(glm::value_ptr(c_deltaTime));
		const __m128 c_gravMulDelta = _mm_mul_ps(c_gravity, c_deltaVec);

		const uint32_t endIndex = container.AliveParticles();
		for (uint32_t i = 0; i < endIndex; ++i)
		{
			__m128& v = container.Velocities().GetValue(i);
			v = _mm_add_ps(v, c_gravMulDelta);
		}
	}

	void EulerPositionUpdater::Update(double deltaTime, ParticleContainer& container)
	{
		__declspec(align(16)) const glm::vec4 c_deltaTime((float)deltaTime);
		const __m128 c_deltaVec = _mm_load_ps(glm::value_ptr(c_deltaTime));

		const uint32_t endIndex = container.AliveParticles();
		for (uint32_t i = 0; i < endIndex; ++i)
		{
			const __m128 p = container.Positions().GetValue(i);
			const __m128 v = container.Velocities().GetValue(i);
			const __m128 vMulDelta = _mm_mul_ps(v, c_deltaVec);
			_mm_stream_ps((float*)&container.Positions().GetValue(i), _mm_add_ps(p, vMulDelta));
		}
		_mm_sfence();
	}

	void KillOnZeroLife::Update(double deltaTime, ParticleContainer& container)
	{
		uint32_t currentP = 0;
		const float c_deltaT = (float)deltaTime;
		while (currentP < container.AliveParticles())
		{
			float& currentLife = container.Lifetimes().GetValue(currentP);
			currentLife -= c_deltaT;

			if (currentLife > 0.0f)
			{
				++currentP;
			}
			else
			{
				container.Kill(currentP);
			}
		}
	}

	void DebugParticleRenderer::Render(double deltaTime, const ParticleContainer& container)
	{
		const uint32_t endIndex = container.AliveParticles();
		__declspec(align(16)) glm::vec4 positionVec;
		for (uint32_t i = 0; i < endIndex; ++i)
		{
			_mm_store_ps(glm::value_ptr(positionVec), container.Positions().GetValue(i));
			m_debugRender->AddAxisAtPoint(positionVec, 0.05f);
		}
	}
		
	bool LiveForever::ShouldKill(double deltaTime, ParticleEffect& effect)
	{
		return false;
	}

	bool KillOnZeroParticles::ShouldKill(double deltaTime, ParticleEffect& effect)
	{
		return effect.AliveParticles() == 0;
	}
}