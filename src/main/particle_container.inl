

inline ParticleContainer::ParticleContainer(uint32_t maxParticles)
{
	Create(maxParticles);
}

inline ParticleContainer::ParticleContainer()
	: m_maxParticles(0)
	, m_livingParticles(0)
{
}

inline ParticleContainer::~ParticleContainer()
{
}

inline void ParticleContainer::Create(uint32_t maxParticles)
{
	m_maxParticles = maxParticles;
	m_livingParticles = 0;

	m_position.Create(maxParticles);
	m_lifetime.Create(maxParticles);
	m_velocity.Create(maxParticles);
}

inline uint32_t ParticleContainer::Wake(uint32_t count)
{
	SDE_ASSERT(m_livingParticles + count < m_maxParticles);
	const uint32_t newIndex = m_livingParticles;

	const uint32_t pIndex = m_position.Wake(count);
	SDE_ASSERT(pIndex == newIndex);

	const uint32_t lIndex = m_lifetime.Wake(count);
	SDE_ASSERT(lIndex == newIndex);

	const uint32_t vIndex = m_velocity.Wake(count);
	SDE_ASSERT(vIndex == newIndex);

	m_livingParticles += count;

	return newIndex;
}

inline void ParticleContainer::Kill(uint32_t index)
{
	SDE_ASSERT(m_livingParticles + index < m_maxParticles);
	if (m_livingParticles + index < m_maxParticles && m_livingParticles > 0)
	{
		m_position.Kill(index);
		m_lifetime.Kill(index);
		m_velocity.Kill(index);

		--m_livingParticles;
	}
}