/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"

#include <JavaScriptCore/JSArrayBufferView.h>
#include <JavaScriptCore/JSArrayBuffer.h>
#include <JavaScriptCore/JSTypedArrays.h>
#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS_ARRAY_BUFFER_VIEW() v8::jscshim::GetJscCellFromV8<JSC::JSArrayBufferView>(this)

/* TODO: In v8's TYPED_ARRAY_NEW macro (api.cc) there's also a check that length doesn't exceed
 * the max length. Do we have a max length and need to check it here? */
#define DEFINE_NEW_TYPED_ARRAY_FUNCTION(type,arrayBufferType) \
	Local<type##Array> type##Array::New(Local<arrayBufferType> array_buffer, size_t byte_offset, size_t length) \
	{ \
		JSC::ExecState * exec = jscshim::GetCurrentExecState(); \
		\
		JSC::JSArrayBuffer* jsBuffer = jscshim::GetJscCellFromV8<JSC::JSArrayBuffer>(*array_buffer); \
		JSC::JSObject * result = JSC::JS##type##Array::create(exec, \
															  exec->lexicalGlobalObject()->typedArrayStructure(JSC::Type##type), \
															  jsBuffer->impl(), \
															  byte_offset, \
															  length); \
		\
		return Local<type##Array>(JSC::JSValue(result)); \
	}

#define DEFINE_NEW_TYPED_ARRAY_FUNCTIONS(type) DEFINE_NEW_TYPED_ARRAY_FUNCTION(type, ArrayBuffer) \
											   DEFINE_NEW_TYPED_ARRAY_FUNCTION(type, SharedArrayBuffer)

namespace v8
{

Local<ArrayBuffer> ArrayBufferView::Buffer()
{
	JSC::JSArrayBuffer * arrayBuffer = GET_JSC_THIS_ARRAY_BUFFER_VIEW()->possiblySharedJSBuffer(jscshim::GetCurrentExecState());
	return Local<ArrayBuffer>(JSC::JSValue(arrayBuffer));
}

size_t ArrayBufferView::ByteOffset()
{
	return GET_JSC_THIS_ARRAY_BUFFER_VIEW()->byteOffset();
}

size_t ArrayBufferView::ByteLength()
{
	JSC::JSArrayBufferView * jscThis = GET_JSC_THIS_ARRAY_BUFFER_VIEW();

	// Based on JSC's (API) JSObjectGetTypedArrayByteLength
	return jscThis->length() * JSC::elementSize(jscThis->classInfo(*jscThis->vm())->typedArrayStorageType);
}

size_t TypedArray::Length()
{
	return GET_JSC_THIS_ARRAY_BUFFER_VIEW()->length();
}

DEFINE_NEW_TYPED_ARRAY_FUNCTIONS(Uint8)
DEFINE_NEW_TYPED_ARRAY_FUNCTIONS(Uint8Clamped)
DEFINE_NEW_TYPED_ARRAY_FUNCTIONS(Int8)
DEFINE_NEW_TYPED_ARRAY_FUNCTIONS(Uint16)
DEFINE_NEW_TYPED_ARRAY_FUNCTIONS(Int16)
DEFINE_NEW_TYPED_ARRAY_FUNCTIONS(Int32)
DEFINE_NEW_TYPED_ARRAY_FUNCTIONS(Uint32)
DEFINE_NEW_TYPED_ARRAY_FUNCTIONS(Float32)
DEFINE_NEW_TYPED_ARRAY_FUNCTIONS(Float64)

Local<BigInt64Array> BigInt64Array::New(Local<ArrayBuffer> array_buffer, size_t byte_offset, size_t length)
{
	// TODO: IMPLEMENT
	return Local<BigInt64Array>();
}

Local<BigInt64Array> BigInt64Array::New(Local<SharedArrayBuffer> shared_array_buffer, size_t byte_offset, size_t length)
{
	// TODO: IMPLEMENT
	return Local<BigInt64Array>();
}

Local<BigUint64Array> BigUint64Array::New(Local<ArrayBuffer> array_buffer, size_t byte_offset, size_t length)
{
	// TODO: IMPLEMENT
	return Local<BigUint64Array>(JSC::jsUndefined());
}
Local<BigUint64Array> BigUint64Array::New(Local<SharedArrayBuffer> shared_array_buffer, size_t byte_offset, size_t length)
{
	// TODO: IMPLEMENT
	return Local<BigUint64Array>(JSC::jsUndefined());
}

} // v8