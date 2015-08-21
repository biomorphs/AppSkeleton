#include "particle_effects.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

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

	void GenerateRandomLifetime::Generate(double deltaTime, ParticleContainer& container, uint32_t startIndex, uint32_t endIndex)
	{
		for (uint32_t i = startIndex; i < endIndex; ++i)
		{
			container.Lifetimes().SetValue(i, m_timeMin + (((float)rand() / (float)RAND_MAX) * (m_timeMax - m_timeMin)));
		}
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

	void ColourFader::Update(double deltaTime, ParticleContainer& container)
	{
		const __m128 c_col0 = _mm_load_ps(glm::value_ptr(m_c0));
		const __m128 c_col1 = _mm_load_ps(glm::value_ptr(m_c1));
		const uint32_t endIndex = container.AliveParticles();
		for (uint32_t i = 0; i < endIndex; ++i)
		{
			float lifetime = m_lifetimeEnd - container.Lifetimes().GetValue(i);
			lifetime = std::max(m_lifetimeStart, lifetime);
			lifetime = std::min(m_lifetimeEnd, lifetime);
			float t = (lifetime - m_lifetimeStart) / (m_lifetimeEnd - m_lifetimeStart);
			const __m128 tVec = { t, t, t, t };
			__m128 colour = _mm_sub_ps(c_col1, c_col0);
			colour = _mm_mul_ps(colour, tVec);
			colour = _mm_add_ps(colour, c_col0);
			_mm_stream_ps((float*)&container.Colours().GetValue(i), colour);
		}
		_mm_sfence();
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

	void EulerFloorBouncer::Update(double deltaTime, ParticleContainer& container)
	{
		const __m128 c_deltaTime = { (float)deltaTime, (float)deltaTime, (float)deltaTime, (float)deltaTime };
		const __m128 c_floor = { -10000000.0f, m_floorHeight, -10000000.0f, -10000000.0f };
		const __m128 c_two = { 0.0f, 2.0f, 0.0f, 0.0f };
		const __m128 c_bounceFactor = { 0.8f, 0.8f, 0.8f, 0.0f };
		const __m128i c_yMask = _mm_set_epi32(0x00000000, 0x00000000, 0xf0000000, 0x00000000);
		const __m128i c_xzMask = _mm_set_epi32(0xf0000000, 0xf0000000, 0x00000000, 0xf0000000);
		const __m128i c_signMask = _mm_set_epi32( 0x00000000, 0x00000000, 0x80000000, 0x00000000 );
		const __m128 c_one = _mm_set_ps(1.0f, 1.0f, 1.0f, 1.0f);
		const uint32_t endIndex = container.AliveParticles();
		for (uint32_t i = 0; i < endIndex; ++i)
		{
			__m128 p = container.Positions().GetValue(i);
			__m128 v = container.Velocities().GetValue(i);
			__m128 velXZ = _mm_maskload_ps((const float*)&v, c_xzMask);
			__m128 velY = _mm_maskload_ps((const float*)&v, c_yMask);
			__m128i belowFloor = _mm_castps_si128(_mm_cmple_ps(p, c_floor));	// 0,0xfffffff,0,0 if <floor

			// flip sign bit if <floor (0,ffffffff,0,0)
			__m128 signFlipMask = _mm_maskload_ps((const float*)&c_signMask, belowFloor);
			__m128 adjustedVelocity = _mm_andnot_ps(signFlipMask, velY);

			velY = adjustedVelocity;
			v = _mm_add_ps(velXZ, velY);

			//__m128 velocityToAdd = _mm_maskload_ps((const float*)&v, belowFloor);	// load velocity.y if belowFloor

			// Strip sign bit of velocity to add, giving us abs(velocity.y)
			//static const __m128 SIGNMASK = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
			//__m128 absval = _mm_andnot_ps(SIGNMASK, velocityToAdd);

			//velocityToAdd = _mm_mul_ps(absval, c_two);	// Adding 2x abs(y) will flip the sign to positive when under the floor
			//v = _mm_add_ps(velocityToAdd, v);

			// clamp position to floor
			p = _mm_max_ps(p, c_floor);

			// integrate
			const __m128 vMulDelta = _mm_mul_ps(v, c_deltaTime);
			const __m128 newPos = _mm_add_ps(p, vMulDelta);

			_mm_stream_ps((float*)&container.Positions().GetValue(i), newPos);
			_mm_stream_ps((float*)&container.Velocities().GetValue(i), v);
		}
		_mm_sfence();
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