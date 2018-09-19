/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "v8.h"

#ifndef MUST_USE_RESULT
	#ifdef __GNUC__
		#define MUST_USE_RESULT __attribute__((warn_unused_result))
	#else
		#define MUST_USE_RESULT
	#endif
#endif

#define DECL_CAST(type) \
  static type* cast(Object* object) { return reinterpret_cast<type*>(object); } \
  static const type* cast(const Object* object) {reinterpret_cast<const type*>(object); }


namespace v8 { namespace internal
{

class CompilationCache;
class Isolate;
class JSArray;
class JSArrayBufferView;
class JSReceiver;
class JSTypedArray;
template <typename T> class MaybeHandle;
class Object;

extern bool FLAG_harmony_sharedarraybuffer;
extern bool FLAG_allow_natives_syntax;

const int kApiPointerSize = sizeof(void*);  // NOLINT

template <size_t ptr_size> struct SmiTagging;

// Smi constants for 32-bit systems.
template <> struct SmiTagging<4> {
	enum { kSmiShiftSize = 0, kSmiValueSize = 31 };
};

// Smi constants for 64-bit systems.
template <> struct SmiTagging<8> {
	enum { kSmiShiftSize = 31, kSmiValueSize = 32 };
};

typedef SmiTagging<kApiPointerSize> PlatformSmiTagging;
const int kSmiShiftSize = PlatformSmiTagging::kSmiShiftSize;
const int kSmiValueSize = PlatformSmiTagging::kSmiValueSize;

enum ElementsKind {
	PACKED_SMI_ELEMENTS,
	PACKED_ELEMENTS,
	PACKED_DOUBLE_ELEMENTS,

	// Fixed typed arrays.
	UINT8_ELEMENTS,
	INT8_ELEMENTS,
	UINT16_ELEMENTS,
	INT16_ELEMENTS,
	UINT32_ELEMENTS,
	INT32_ELEMENTS,
	FLOAT32_ELEMENTS,
	FLOAT64_ELEMENTS,
	UINT8_CLAMPED_ELEMENTS,

	NO_ELEMENTS
};

inline int SNPrintF(Vector<char> str, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	int result = VSNPrintF(str, format, args);
	va_end(args);
	return result;
}

inline int VSNPrintF(Vector<char> str, const char* format, va_list args)
{
	return base::OS::VSNPrintF(str.start(), str.length(), format, args);
}

class Factory final
{
private:
	Isolate * m_isolate;

public:
	Factory(Isolate * isolate) : m_isolate(isolate) {}

	Handle<JSTypedArray> NewJSTypedArray(ElementsKind elements_kind,
										 size_t number_of_elements,
										 PretenureFlag pretenure = NOT_TENURED);
};

class CompilationCache
{
public:
	void Clear() {}
};

/* We don't really have a seperate "internal" Isolate, so this is just a "dummy" for the UT. 
 * Thus, this class can't have any data members of it's own. */
class Isolate : public v8::Isolate
{
public:
	Factory * factory();
	CompilationCache* compilation_cache() { return nullptr; }

	Context * context();

	bool has_pending_exception();

#ifdef DEBUG
	static size_t non_disposed_isolates();
#endif
};

template <typename T>
class Handle
{
private:
	friend class Factory;
	template <typename S> friend class Handle;
	template <typename S> friend class MaybeHandle;
	friend class v8::Utils;

	JSC::JSValue m_value;

public:
	explicit Handle(T * val) :
		m_value(*reinterpret_cast<JSC::JSValue *>(val))
	{
	}

	Handle(const Handle<T>& that) :
		m_value(that.m_value)
	{
	}
	
	template <typename S>
	Handle(const Handle<S>& that) :
		m_value(that.m_value)
	{
	}

	/* (jscshim) Make sure we clear the value, since th GC scans the stack,
	 * and might see our value even after our destructor will be called. */
	~Handle()
	{
		m_value = JSC::JSValue();
	}

	Handle<T>& operator=(const Handle<T>& rhs)
	{
		m_value = rhs.m_value;
		return *this;
	}

	T* operator->() const { return reinterpret_cast<T *>(const_cast<JSC::JSValue *>(&m_value)); }

	T* operator*() const { return reinterpret_cast<T *>(const_cast<JSC::JSValue *>(&m_value)); }

	template <typename S>
	static const Handle<T> cast(Handle<S> that) {
		// TODO: Add checks?
		return Handle<T>(that.m_value);
	}

private:
	explicit Handle(const JSC::JSValue& that) : m_value(that) {}
};

template <typename T>
class MaybeHandle final
{
private:
	template <typename S> friend class MaybeHandle;
	friend class Object;
	friend class JSReceiver;
	JSC::JSValue m_value;

public:
	MaybeHandle() {}

	template <typename S>
	MaybeHandle(const Handle<S>& handle) :
		m_value(handle.m_value)
	{
	}

	MaybeHandle(const MaybeHandle<T>& that) :
		m_value(that.m_value)
	{
	}

	template <typename S>
	MaybeHandle(MaybeHandle<S> maybe_handle) :
		m_value(maybe_handle.m_value)
	{
	}

	/* (jscshim) Make sure we clear the value, since th GC scans the stack,
	 * and might see our value even after our destructor will be called. */
	~MaybeHandle()
	{
		m_value = JSC::JSValue();
	}

	V8_INLINE MaybeHandle<T>& operator=(const MaybeHandle<T>& rhs)
	{
		this->val_ = rhs.val_;
		return *this;
	}

	V8_INLINE void Check() const
	{ 
		CHECK(!m_value.isEmpty());
	}

	V8_INLINE Handle<T> ToHandleChecked() const
	{
		Check();
		return Handle<T>(m_value);
	}

private:
	explicit MaybeHandle(const JSC::JSValue& that) : m_value(that) {}
};

class Object
{
public:
	// TODO
	Object * elements() { return this; }

	double Number() const;

	MUST_USE_RESULT static MaybeHandle<Object> GetElement(
		Isolate* isolate, Handle<Object> object, uint32_t index);

	bool IsJSGlobalProxy() const;
};

class Smi : public Object
{
public:
	static int ToInt(const Object* object);

	static const int kMinValue =
		(static_cast<unsigned int>(-1)) << (kSmiValueSize - 1);
	static const int kMaxValue = -(kMinValue + 1);
};

class HeapObject : public Object
{
public:
	inline Isolate* GetIsolate() const
	{
		// TODO: Real object's isolate?
		return reinterpret_cast<Isolate *>(v8::Isolate::GetCurrent());
	}

	bool IsJSObject() const;
};

class FixedArrayBase : public  HeapObject
{
public:
	int length() const;

protected:
	void * GetBuffer() const;
};

// Based on v8's implementation found in v8's objects.h
template <typename ElementType>
class FixedTypedArray : public FixedArrayBase
{
public:
	ElementType get_scalar(int index)
	{
		return reinterpret_cast<ElementType *>(GetBuffer())[index];
	}

	void set(int index, ElementType value)
	{
		reinterpret_cast<ElementType *>(GetBuffer())[index] = value;
	}

	DECL_CAST(FixedTypedArray<ElementType>);
};

using FixedUint8Array = FixedTypedArray<uint8_t>;
using FixedInt8Array = FixedTypedArray<int8_t>;
using FixedUint16Array = FixedTypedArray<uint16_t>;
using FixedInt16Array = FixedTypedArray<int16_t>;
using FixedUint32Array = FixedTypedArray<uint32_t>;
using FixedInt32Array = FixedTypedArray<int32_t>;
using FixedFloat32Array = FixedTypedArray<float>;
using FixedFloat64Array = FixedTypedArray<double>;
using FixedUint8ClampedArray = FixedTypedArray<uint8_t>;

class JSReceiver : public HeapObject
{
	MUST_USE_RESULT static MaybeHandle<Object> GetElement(
		Isolate* isolate, Handle<JSReceiver> receiver, uint32_t index);
};

class JSObject : public JSReceiver 
{
public:
	ElementsKind GetElementsKind();
};

class JSArray : public JSObject {};

class JSArrayBufferView : public JSObject {};

class JSTypedArray : public JSArrayBufferView {};

}} // v8::internal