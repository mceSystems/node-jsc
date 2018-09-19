/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"
#include "shim/ArrayBufferHelpers.h"
#include <JavaScriptCore/JSCInlines.h>

namespace v8
{

Local<ArrayBuffer> ArrayBuffer::New(Isolate* isolate, size_t byte_length)
{
	DECLARE_SHIM_EXCEPTION_SCOPE(isolate);
	return Local<ArrayBuffer>(JSC::JSValue(jscshim::CreateJscArrayBuffer(isolate, byte_length, JSC::ArrayBufferSharingMode::Default)));
}

Local<ArrayBuffer> ArrayBuffer::New(Isolate* isolate,
									void* data, 
									size_t byte_length, 
									ArrayBufferCreationMode mode)
{
	DECLARE_SHIM_EXCEPTION_SCOPE(isolate);
	return Local<ArrayBuffer>(JSC::JSValue(jscshim::CreateJscArrayBuffer(isolate,
																		 data,
																		 byte_length,
																		 mode,
																		 JSC::ArrayBufferSharingMode::Default)));
}

size_t ArrayBuffer::ByteLength() const
{
	return GET_JSC_THIS_ARRAY_BUFFER()->byteLength();
}

bool ArrayBuffer::IsNeuterable() const
{
	JSC::ArrayBuffer * thisBuffer = GET_JSC_THIS_ARRAY_BUFFER();
	
	/* From v8's JSArrayBuffer::Setup (objects.cc): array_buffer->set_is_neuterable(shared == SharedFlag::kNotShared);
	 * From JSC's ArrayBuffer::transferTo (runtime/ArrayBuffer.cpp): bool isNeuterable = !m_pinCount && !m_locked;
	 *
	 * For now we don't have access to the m_pinCount member, but we'll mimic the rest (either way,
	 * JSC::ArrayBuffer::neuter will check it).
	 *
	 * TODO: In v8's wasm::SetupArrayBuffer, the buffers is_neuterable is set to false, 
	 * while in JSC's ArrayBuffer::neuter there's a comment saying "We allow neutering 
	 * wasm memory ArrayBuffers even though they are locked.". Should we return false here
	 * for "wasm memory" ArrayBuffers?
	*/
	return (!thisBuffer->isShared() && !thisBuffer->isLocked());
}

void ArrayBuffer::Neuter()
{
	JSC::ArrayBuffer * thisBuffer = GET_JSC_THIS_ARRAY_BUFFER();

	jscshim::ApiCheck(thisBuffer->isApiUserControlledBuffer(),
					  "v8::ArrayBuffer::Neuter",
					  "Only externalized ArrayBuffers can be neutered");
	jscshim::ApiCheck(IsNeuterable(),
					  "v8::ArrayBuffer::Neuter",
					  "Only neuterable ArrayBuffers can be neutered");

	GET_JSC_THIS_ARRAY_BUFFER()->neuter(jscshim::GetCurrentVM());
}

bool ArrayBuffer::IsExternal() const
{
	return GET_JSC_THIS_ARRAY_BUFFER()->isApiUserControlledBuffer();
}

ArrayBuffer::Contents ArrayBuffer::Externalize()
{
	JSC::ArrayBuffer * thisBuffer = GET_JSC_THIS_ARRAY_BUFFER();

	jscshim::ApiCheck(!thisBuffer->isApiUserControlledBuffer(),
					  "v8_ArrayBuffer_Externalize",
					  "ArrayBuffer already externalized");

	thisBuffer->makeApiUserControlledBuffer();
	return { thisBuffer->data(), thisBuffer->byteLength() };
}

ArrayBuffer::Contents ArrayBuffer::GetContents()
{
	JSC::ArrayBuffer * thisBuffer = GET_JSC_THIS_ARRAY_BUFFER();
	return { thisBuffer->data(), thisBuffer->byteLength() };
}

} // v8