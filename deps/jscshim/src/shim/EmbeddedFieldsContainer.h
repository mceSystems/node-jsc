/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include <JavaScriptCore/JSCJSValue.h>
#include <JavaScriptCore/Protect.h>
#include <wtf/Vector.h>
#include <wtf/Compiler.h>

namespace v8 { namespace jscshim
{

// Not that in v8's API, unassigned values are expected to be js "undefined" values
struct EmbeddedField
{
	// TODO: Encode m_isJscValue in the value itself (new tag maybe?) to save some memory?
	union 
	{
		JSC::JSValue value;
		void * rawValue;
	} m_data;

	bool m_isJscValue;

	EmbeddedField() : m_data({ JSC::jsUndefined() }), m_isJscValue(true) {}
};

}} // v8:jscshim

/* Since we'll initialize empty fields to js "undefined" values, we need to initialize new fields
 * in our fields buffer. But, for copying\moving, we can just memcpy\memmove. This Specializing of 
  * WTF::VectorTypeOperations will force WTF::Vector to do that (and use memmove\memcpy when possible).
 * Note that although it a bit of an overhead to do all that to simply store an array of fields,
 * its worth using WTF::Vector since it implements an inline storage for small vectors, thus 
 * will avoid heap allocations for small vectors (automatically). */
template<>
struct WTF::VectorTypeOperations<v8::jscshim::EmbeddedField>
{
	using Field = v8::jscshim::EmbeddedField;

	static void destruct(Field* begin, Field* end) {}

	static void initializeIfNonPOD(Field* begin, Field* end)
	{
		for (Field * current = begin; current != end; ++current)
		{
			new (NotNull, current) Field;
		}
	}

	static void move(Field* src, Field* srcEnd, Field* dst)
	{ 
		memcpy(dst, src, reinterpret_cast<const char*>(srcEnd) - reinterpret_cast<const char*>(src));
	}

	static void moveOverlapping(Field* src, Field* srcEnd, Field* dst)
	{ 
		memmove(dst, src, reinterpret_cast<const char*>(srcEnd) - reinterpret_cast<const char*>(src));
	}

	static void uninitializedCopy(const Field* src, const Field* srcEnd, Field* dst)
	{
		memcpy(dst, src, reinterpret_cast<const char*>(srcEnd) - reinterpret_cast<const char*>(src));
	}

	static void uninitializedFill(Field* dst, Field* dstEnd, const Field& val)
	{
		for (Field * current = dst; current != dstEnd; ++current)
		{
			*current = val;
		}
	}
};

namespace v8 { namespace jscshim
{

/* A container for v8 class' "embedder's data", supporting both Context's embedded data, 
 * which "auto grows" when setting fields, and Object's internal fields, which has a fixed count. 
 *
 * TODO: Cleanup this class? */
template <bool AutoGrow = false>
class EmbeddedFieldsContainer
{
private:
	WTF::Vector<EmbeddedField> m_fields;

public:
	EmbeddedFieldsContainer(int initialSize) :
		m_fields(initialSize)
	{
	}

	size_t Size() const { return m_fields.size(); }

	ALWAYS_INLINE JSC::JSValue GetValue(int index);
	ALWAYS_INLINE void * GetAlignedPointer(int index);

	ALWAYS_INLINE void SetValue(int index, JSC::JSValue value);
	ALWAYS_INLINE void SetAlignedPointer(int index, void  * pointer);

	// CAREFULL! To be used only when the caller has checked the index is within the valid range
	ALWAYS_INLINE EmbeddedField& GetFieldUnsafe(size_t index);

private:
	ALWAYS_INLINE EmbeddedField& GetFieldForOverwrite(int index);
	ALWAYS_INLINE void UnsafeSetField(int index, JSC::JSValue value);
	ALWAYS_INLINE void UnsafeSetField(int index, void * rawPointer);
};

template <bool AutoGrow>
JSC::JSValue EmbeddedFieldsContainer<AutoGrow>::GetValue(int index)
{
	if ((index >= 0) && static_cast<size_t>(index) < m_fields.size())
	{
		ASSERT(m_fields.at(static_cast<size_t>(index)).m_isJscValue);
		return m_fields.at(static_cast<size_t>(index)).m_data.value;
	}

	return JSC::jsUndefined();
}

template <bool AutoGrow>
void * EmbeddedFieldsContainer<AutoGrow>::GetAlignedPointer(int index)
{
	if ((index >= 0) && static_cast<size_t>(index) < m_fields.size())
	{
		ASSERT(false == m_fields.at(static_cast<size_t>(index)).m_isJscValue);
		return m_fields.at(static_cast<size_t>(index)).m_data.rawValue;
	}

	return nullptr;
}

template <>
ALWAYS_INLINE void EmbeddedFieldsContainer<false>::SetValue(int index, JSC::JSValue value)
{
	if ((index >= 0) && static_cast<size_t>(index) < m_fields.size())
	{
		UnsafeSetField(index, value);
	}
}

template <>
ALWAYS_INLINE void EmbeddedFieldsContainer<true>::SetValue(int index, JSC::JSValue value)
{
	if (index < 0)
	{
		return;
	}

	UnsafeSetField(index, value);
}

template <>
ALWAYS_INLINE void EmbeddedFieldsContainer<false>::SetAlignedPointer(int index, void  * pointer)
{
	if ((index >= 0) && static_cast<size_t>(index) < m_fields.size())
	{
		UnsafeSetField(index, pointer);
	}
}

template <bool AutoGrow>
ALWAYS_INLINE EmbeddedField& EmbeddedFieldsContainer<AutoGrow>::GetFieldUnsafe(size_t index)
{
	return m_fields.at(static_cast<size_t>(index));
}

template <>
ALWAYS_INLINE void EmbeddedFieldsContainer<true>::SetAlignedPointer(int index, void  * pointer)
{
	if (index < 0) 
	{
		return;
	}

	UnsafeSetField(index, pointer);
}

template <>
ALWAYS_INLINE EmbeddedField& EmbeddedFieldsContainer<false>::GetFieldForOverwrite(int index)
{
	// If the current value is a JSValue, "release" it from our hands
	EmbeddedField& field = m_fields.at(static_cast<size_t>(index));
	if (field.m_isJscValue)
	{
		JSC::gcUnprotect(field.m_data.value);
	}

	return field;
}

template <>
ALWAYS_INLINE EmbeddedField& EmbeddedFieldsContainer<true>::GetFieldForOverwrite(int index)
{
	size_t minSize = static_cast<size_t>(index) + 1;
	if (minSize > m_fields.size())
	{
		m_fields.grow(minSize);
		return m_fields.at(static_cast<size_t>(index));
	}

	// If the current value is a JSValue, "release" it from our hands
	EmbeddedField& field = m_fields.at(static_cast<size_t>(index));
	if (field.m_isJscValue)
	{
		JSC::gcUnprotect(field.m_data.value);
	}

	return field;
}

template <bool AutoGrow>
ALWAYS_INLINE void EmbeddedFieldsContainer<AutoGrow>::UnsafeSetField(int index, JSC::JSValue value)
{
	EmbeddedField& field = GetFieldForOverwrite(index);
	field.m_data.value = value;
	field.m_isJscValue = true;

	// Make sure the value won't be collected
	JSC::gcProtect(value);
}

template <bool AutoGrow>
ALWAYS_INLINE void EmbeddedFieldsContainer<AutoGrow>::UnsafeSetField(int index, void * rawPointer)
{
	EmbeddedField& field = GetFieldForOverwrite(index);
	field.m_data.rawValue = rawPointer;
	field.m_isJscValue = false;
}

}} // v8::jscshim