#include "render_system.h"
#include "render/window.h"
#include "render/device.h"
#include "kernel/assert.h"

template<class ObjectType, uint32_t MaxObjects>
class StaticBucketAllocator
{
public:
	StaticBucketAllocator()
	{
		memset(m_buffer, 0, c_BucketSize);
		memset(m_freeListBuffer, 0, sizeof(*m_freeListBuffer) * MaxObjects);
		memset(m_usedListBuffer, 0, sizeof(*m_usedListBuffer) * MaxObjects);
	}

	void* AllocateBuffer()
	{
		return m_buffer;
	}

	uint32_t* AllocateFreeList()
	{
		return m_freeListBuffer;
	}

	uint32_t* AllocateUsedList()
	{
		return m_usedListBuffer;
	}

	void Free(void* buffer) { }
private:
	static const size_t c_BucketSize = sizeof(ObjectType)* MaxObjects;
	uint8_t m_buffer[c_BucketSize];
	uint32_t m_freeListBuffer[MaxObjects];
	uint32_t m_usedListBuffer[MaxObjects];
};

template<class ObjectType, uint32_t MaxObjects, class Allocator>
class FastObjectBucket
{
public:
	FastObjectBucket()
		: m_freeCount(0)
		, m_usedCount(0)
		, m_usedListDirty(false)
	{
		m_objectBuffer = (ObjectType*)m_allocator.AllocateBuffer();
		SDE_ASSERT(m_objectBuffer);

		m_freeList = m_allocator.AllocateFreeList();
		SDE_ASSERT(m_freeList);

		m_usedList = m_allocator.AllocateUsedList();
		SDE_ASSERT(m_usedList);

		// Init free list
		for (uint32_t i = 0; i < MaxObjects; ++i)
		{
			// In reverse order as we always pop from the back of the free list
			m_freeList[i] = MaxObjects - 1 - i;
		}
		m_freeCount = MaxObjects;
	}

	~FastObjectBucket()
	{
		SDE_ASSERT(m_usedCount == 0);
		m_allocator.Free(m_objectBuffer);
		m_allocator.Free(m_freeList);
		m_allocator.Free(m_usedList);
	}

	ObjectType* AddObject()
	{
		SDE_ASSERT(m_freeCount > 0);
		if (m_freeCount == 0)
		{
			return nullptr;
		}

		// pop a new free one from the list
		uint32_t index = m_freeList[--m_freeCount];

		// add it to the used list
		m_usedList[m_usedCount++] = index;
		m_usedListDirty = true;

		// default-construct the object
		new (&m_objectBuffer[index]) ObjectType;
		return &(m_objectBuffer[index]);
	}

	void RemoveObject(ObjectType* theObject)
	{
		SDE_ASSERT(m_usedCount>0);

		// calculate the index into the buffer
		uint32_t objectIndex = (uint32_t)(theObject - m_objectBuffer);
		SDE_ASSERT(objectIndex < MaxObjects);

		// remove the object from the used list
		for (uint32_t i = 0; i < m_usedCount; ++i)
		{
			if (m_usedList[i] == objectIndex)
			{
				// swap this index with the back one
				m_usedList[i] = m_usedList[--m_usedCount];
				m_usedListDirty = true;
				break;
			}
		}

		// add the index to the back of the free list
		m_freeList[m_freeCount++] = objectIndex;

		// Finally, call the object destructor
		theObject->~ObjectType();
	}

private:
	Allocator m_allocator;
	ObjectType* m_objectBuffer;
	uint32_t* m_freeList;
	uint32_t m_freeCount;
	uint32_t* m_usedList;
	uint32_t m_usedCount;
	bool m_usedListDirty;		// track if we need to resort the used list for iteration
};

class RefcountedData
{
public:
	RefcountedData()
		: m_data(nullptr)
		, m_refCount(0)
	{
	}

	explicit RefcountedData(void* data)
		: m_data(data)
		, m_refCount(0)
	{
	}

	~RefcountedData()
	{
		m_data = nullptr;
	}

	RefcountedData(RefcountedData&& other)
	{
		*this = std::move(other);
	}

	RefcountedData& operator=(RefcountedData&& other)
	{
		m_data = other.m_data;
		other.m_data = nullptr;
		return *this;
	}

	void AddReference()
	{
		++m_refCount;
	}

	void RemoveReference()
	{
		--m_refCount;
	}

	void* GetData()
	{
		return m_data;
	}

private:
	RefcountedData(const RefcountedData& other);
	RefcountedData& operator=(const RefcountedData& other);

	void* m_data;
	int32_t m_refCount;
};

template< class Internal, class DataType >
class Handle
{
public:
	Handle()
		: m_internalData(nullptr)
	{

	}

	Handle(const Handle& other)
	{
		*this = other;
	}

	Handle(Handle&& other)
	{
		*this = std::move(other);
	}

	explicit Handle(Internal* internalData)
	{
		m_internalData = internalData;
		if (m_internalData)
			m_internalData->AddReference();
	}

	Handle& operator=(const Handle& other)
	{
		if (m_internalData)
			m_internalData->RemoveReference();

		m_internalData = other.m_internalData;
		if (m_internalData)
			m_internalData->AddReference();

		return *this;
	}

	Handle& operator=(Handle&& other)
	{
		if (m_internalData)
			m_internalData->RemoveReference();

		m_internalData = other.m_internalData;
		other.m_internalData = nullptr;

		return *this;
	}

	~Handle()
	{
		if (m_internalData)
			m_internalData->RemoveReference();
		m_internalData = nullptr;
	}

	DataType* operator*()
	{
		if (m_internalData)
			return reinterpret_cast<DataType*>( m_internalData->GetData() );

		return nullptr;
	}

	const DataType* operator*() const
	{
		if (m_internalData)
			return reinterpret_cast<const DataType*>(m_internalData->GetData());

		return nullptr;
	}

private:
	Internal* m_internalData;
};

RenderSystem::RenderSystem()
: m_quit(false)
, m_window(nullptr)
, m_device(nullptr)
, m_textureCache(nullptr)
{
	typedef FastObjectBucket<RefcountedData, 4, StaticBucketAllocator< RefcountedData, 4 > > RefDataBucket;
	typedef Handle<RefcountedData, int> SimpleIntHandle;

	int x = 2;
	int y = 4;
	int z = 8;
	int w = 16;

	RefDataBucket theBucket;

	RefcountedData* wData = theBucket.AddObject();
	*wData = std::move(RefcountedData(&w));

	RefcountedData* xData = theBucket.AddObject();
	*xData = std::move(RefcountedData(&x));

	RefcountedData* yData = theBucket.AddObject();
	*yData = std::move(RefcountedData(&y));

	RefcountedData* zData = theBucket.AddObject();
	*zData = std::move(RefcountedData(&z));

	{
		SimpleIntHandle xHandle(xData);
		SimpleIntHandle yHandle(yData);
		SimpleIntHandle zHandle(zData);

		{
			SimpleIntHandle xHandleCopy = xHandle;
			SimpleIntHandle xHandleCopy2 = xHandle;
			SimpleIntHandle yHandleCopy = yHandle;
			yHandleCopy = xHandleCopy;

			SimpleIntHandle zHandleCopy = zHandle;
			zHandleCopy = yHandleCopy;
		}
	}

	theBucket.RemoveObject(zData);
	theBucket.RemoveObject(xData);
	theBucket.RemoveObject(wData);
	theBucket.RemoveObject(yData);

	theBucket.AddObject();
	theBucket.AddObject();
	theBucket.AddObject();
	theBucket.AddObject();
	theBucket.AddObject();
}

bool RenderSystem::PreInit(Core::ISystemEnumerator& systemEnumerator)
{
	m_window = new Render::Window(Render::Window::Properties("Skeleton Application", 640, 480));
	m_device = new Render::Device(*m_window);
	m_textureCache = new Render::TextureCache(m_device);
	m_window->Show();

	return true;
}

void RenderSystem::OnEventRecieved(const Core::EngineEvent& e)
{
	if (e.m_type == Core::EngineEvent::QuitRequest)
	{
		m_quit = true;
	}
}

bool RenderSystem::Tick()
{
	m_device->Present();
	return !m_quit;
}

void RenderSystem::Shutdown()
{
	m_window->Hide();
	delete m_textureCache;
	delete m_device;
	delete m_window;
}