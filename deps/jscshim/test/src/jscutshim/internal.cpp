/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#define BUILDING_JSCSHIM
#include "v8-pollyfill.h"

#include "../../src/shim/helpers.h"

#include <JavaScriptCore/JSArrayBufferView.h>
#include <JavaScriptCore/ArrayBuffer.h>
#include <JavaScriptCore/JSTypedArrays.h>
#include <JavaScriptCore/JSProxy.h>
#include <JavaScriptCore/JSGlobalObject.h>
#include <JavaScriptCore/CatchScope.h>
#include <JavaScriptCore/ThrowScope.h>
#include <JavaScriptCore/JSCInlines.h>
#include <JavaScriptCore/JSGenericTypedArrayViewInlines.h>

#ifndef MUST_USE_RESULT
	#ifdef __GNUC__
		#define MUST_USE_RESULT __attribute__((warn_unused_result))
	#else
		#define MUST_USE_RESULT
	#endif
#endif

namespace
{

WTF::HashMap<const v8::internal::Isolate *, v8::internal::Factory *> g_factories;

v8::internal::Factory * GetFactoryForIsolate(v8::internal::Isolate * isolate)
{
	v8::internal::Factory * factory = g_factories.inlineGet(isolate);
	if (factory)
	{
		return factory;
	}

	factory = new v8::internal::Factory(isolate);
	g_factories.add(isolate, factory);

	return factory;
}

JSC::TypedArrayType v8ElementTypeToJscArrayType(v8::internal::ElementsKind elementsKind)
{
	using ElementsKind = v8::internal::ElementsKind;

	switch (elementsKind)
	{
	case ElementsKind::INT8_ELEMENTS:
		return JSC::TypeInt8;
	case ElementsKind::INT16_ELEMENTS:
		return JSC::TypeInt16;
	case ElementsKind::INT32_ELEMENTS:
		return JSC::TypeInt32;
	case ElementsKind::UINT8_ELEMENTS:
		return JSC::TypeUint8;
	case ElementsKind::UINT8_CLAMPED_ELEMENTS:
		return JSC::TypeUint8Clamped;
	case ElementsKind::UINT16_ELEMENTS:
		return JSC::TypeUint16;
	case ElementsKind::UINT32_ELEMENTS:
		return JSC::TypeUint32;
	case ElementsKind::FLOAT32_ELEMENTS:
		return JSC::TypeFloat32;
	case ElementsKind::FLOAT64_ELEMENTS:
		return JSC::TypeFloat64;
	default:
		RELEASE_ASSERT_NOT_REACHED();
	}
}

}

namespace v8 { namespace internal
{

bool FLAG_harmony_sharedarraybuffer = true;
bool FLAG_allow_natives_syntax = true;

// Based on JSC's createTypedArray (JSTypedArray.cpp)
Handle<JSTypedArray> Factory::NewJSTypedArray(ElementsKind elements_kind,
											  size_t number_of_elements,
											  PretenureFlag pretenure)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Isolate(m_isolate);
	auto scope = DECLARE_THROW_SCOPE(exec->vm());

	JSC::JSGlobalObject* globalObject = exec->lexicalGlobalObject();

	unsigned int elementByteSize = JSC::elementSize(v8ElementTypeToJscArrayType(elements_kind));

	auto buffer = JSC::ArrayBuffer::tryCreate(number_of_elements, elementByteSize);
	
	JSC::JSArrayBufferView * newBuffer = nullptr;
	switch (elements_kind) {
	case INT8_ELEMENTS:
		newBuffer = JSC::JSInt8Array::create(exec, globalObject->typedArrayStructure(JSC::TypeInt8), WTFMove(buffer), 0, number_of_elements);
		break;
	case INT16_ELEMENTS:
		newBuffer = JSC::JSInt16Array::create(exec, globalObject->typedArrayStructure(JSC::TypeInt16), WTFMove(buffer), 0, number_of_elements);
		break;
	case INT32_ELEMENTS:
		newBuffer = JSC::JSInt32Array::create(exec, globalObject->typedArrayStructure(JSC::TypeInt32), WTFMove(buffer), 0, number_of_elements);
		break;
	case UINT8_ELEMENTS:
		newBuffer = JSC::JSUint8Array::create(exec, globalObject->typedArrayStructure(JSC::TypeUint8), WTFMove(buffer), 0, number_of_elements);
		break;
	case UINT8_CLAMPED_ELEMENTS:
		newBuffer = JSC::JSUint8ClampedArray::create(exec, globalObject->typedArrayStructure(JSC::TypeUint8Clamped), WTFMove(buffer), 0, number_of_elements);
		break;
	case UINT16_ELEMENTS:
		newBuffer = JSC::JSUint16Array::create(exec, globalObject->typedArrayStructure(JSC::TypeUint16), WTFMove(buffer), 0, number_of_elements);
		break;
	case UINT32_ELEMENTS:
		newBuffer = JSC::JSUint32Array::create(exec, globalObject->typedArrayStructure(JSC::TypeUint32), WTFMove(buffer), 0, number_of_elements);
		break;
	case FLOAT32_ELEMENTS:
		newBuffer = JSC::JSFloat32Array::create(exec, globalObject->typedArrayStructure(JSC::TypeFloat32), WTFMove(buffer), 0, number_of_elements);
		break;
	case FLOAT64_ELEMENTS:
		newBuffer = JSC::JSFloat64Array::create(exec, globalObject->typedArrayStructure(JSC::TypeFloat64), WTFMove(buffer), 0, number_of_elements);
		break;
	default:
		RELEASE_ASSERT_NOT_REACHED();
	}

	return Handle<JSTypedArray>(JSC::JSValue(newBuffer));
}

Factory * Isolate::factory()
{
	return GetFactoryForIsolate(this);
}

Context * Isolate::context()
{
	 return reinterpret_cast<Context *>(jscshim::V8IsolateToJscShimIsolate(this)->GetCurrentContext());
}

bool Isolate::has_pending_exception()
{
	JSC::VM& vm = jscshim::V8IsolateToJscShimIsolate(this)->VM();
	auto catchScope = DECLARE_CATCH_SCOPE(vm);

	return (nullptr != catchScope.exception());
}

#ifdef DEBUG
size_t Isolate::non_disposed_isolates()
{
	return jscshim::Isolate::NonDisposedIsolates();
}
#endif

double Object::Number() const
{
	return jscshim::GetValue(this).asNumber();
}

MaybeHandle<Object> Object::GetElement(Isolate* isolate, Handle<Object> object, uint32_t index)
{
	JSC::JSObject * obj = jscshim::GetValue(*object).getObject();
	ASSERT(obj);

	JSC::JSValue element = obj->get(jscshim::GetExecStateForV8Isolate(isolate), index);
	return MaybeHandle<Object>(element.isUndefined() ? JSC::JSValue() : element);
}

bool Object::IsJSGlobalProxy() const
{
	JSC::VM& vm = jscshim::GetCurrentVM();

	JSC::JSProxy * obj = JSC::jsDynamicCast<JSC::JSProxy *>(vm, jscshim::GetValue(this));
	if (nullptr == obj)
	{
		return false;
	}

	return obj->target()->inherits(vm, JSC::JSGlobalObject::info());
}

MaybeHandle<Object> JSReceiver::GetElement(Isolate* isolate, Handle<JSReceiver> receiver, uint32_t index)
{
	return Object::GetElement(isolate, Handle<Object>(receiver), index);
}

ElementsKind JSObject::GetElementsKind()
{
	JSC::JSObject * thisObj = jscshim::GetValue(this).getObject();

	ElementsKind kind = NO_ELEMENTS;
	switch (thisObj->indexingType() & JSC::IndexingShapeMask)
	{
	case JSC::NoIndexingShape:
		kind = NO_ELEMENTS;
		break;
	case JSC::Int32Shape:
		kind = PACKED_SMI_ELEMENTS;
		break;
	case JSC::DoubleShape:
		kind = PACKED_DOUBLE_ELEMENTS;
		break;
	default:
		kind = PACKED_ELEMENTS;
		break;
	}

	return kind;
}

bool HeapObject::IsJSObject() const
{
	return jscshim::GetValue(this).isObject();
}

int Smi::ToInt(const Object* object)
{
	return jscshim::GetValue(object).asInt32();
}

void * FixedArrayBase::GetBuffer() const
{
	JSC::JSArrayBufferView * thisArray = v8::jscshim::GetJscCellFromV8<JSC::JSArrayBufferView>(this);

	// Note the "vector" already accounts for the byte offset
	return thisArray->vector();
}

int FixedArrayBase::length() const
{
	JSC::JSArrayBufferView * thisArray = v8::jscshim::GetJscCellFromV8<JSC::JSArrayBufferView>(this);

	return thisArray->length();
}

}}