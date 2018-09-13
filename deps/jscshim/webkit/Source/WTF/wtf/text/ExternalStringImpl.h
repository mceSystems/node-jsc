#ifndef ExternalStringImpl_h
#define ExternalStringImpl_h

#include <wtf/text/StringImpl.h>
#include <wtf/Function.h>

namespace WTF {

class ExternalStringImpl;

typedef Function<void(ExternalStringImpl*, void*, unsigned)> ExternalStringImplFreeFunction;

class ExternalStringImpl : public StringImpl {
public:
	WTF_EXPORT_PRIVATE static Ref<ExternalStringImpl> create(const LChar * characters, unsigned length, ExternalStringImplFreeFunction&& free);
	WTF_EXPORT_PRIVATE static Ref<ExternalStringImpl> create(const UChar * characters, unsigned length, ExternalStringImplFreeFunction&& free);

private:
	friend class StringImpl;

	ExternalStringImpl(const LChar * characters, unsigned length, ExternalStringImplFreeFunction&& free);
	ExternalStringImpl(const UChar * characters, unsigned length, ExternalStringImplFreeFunction&& free);

	ALWAYS_INLINE void FreeExternalBuffer(void * buffer, unsigned bufferSize)
	{
		m_free(this, buffer, bufferSize);
	}

	ExternalStringImplFreeFunction m_free;
};

} // namespace WTF

using WTF::ExternalStringImpl;

#endif // ExternalStringImpl_h
