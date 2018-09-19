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

Local<SharedArrayBuffer> SharedArrayBuffer::New(Isolate * isolate, size_t byte_length)
{
	DECLARE_SHIM_EXCEPTION_SCOPE(isolate);
	return Local<SharedArrayBuffer>(JSC::JSValue(jscshim::CreateJscArrayBuffer(isolate, byte_length, JSC::ArrayBufferSharingMode::Shared)));
}

Local<SharedArrayBuffer> SharedArrayBuffer::New(Isolate * isolate, void* data, size_t byte_length, ArrayBufferCreationMode mode)
{
	DECLARE_SHIM_EXCEPTION_SCOPE(isolate);
	return Local<SharedArrayBuffer>(JSC::JSValue(jscshim::CreateJscArrayBuffer(isolate,
																			   data,
																			   byte_length,
																			   mode,
																			   JSC::ArrayBufferSharingMode::Shared)));
}

size_t SharedArrayBuffer::ByteLength() const
{
	return GET_JSC_THIS_ARRAY_BUFFER()->byteLength();
}

bool SharedArrayBuffer::IsExternal() const
{
	return GET_JSC_THIS_ARRAY_BUFFER()->isApiUserControlledBuffer();
}

SharedArrayBuffer::Contents SharedArrayBuffer::Externalize()
{
	JSC::ArrayBuffer * thisBuffer = GET_JSC_THIS_ARRAY_BUFFER();

	jscshim::ApiCheck(!thisBuffer->isApiUserControlledBuffer(),
					  "v8_SharedArrayBuffer_Externalize",
					  "SharedArrayBuffer already externalized");

	thisBuffer->makeApiUserControlledBuffer();
	return { thisBuffer->data(), thisBuffer->byteLength() };
}

SharedArrayBuffer::Contents SharedArrayBuffer::GetContents()
{
	JSC::ArrayBuffer * thisBuffer = GET_JSC_THIS_ARRAY_BUFFER();
	return { thisBuffer->data(), thisBuffer->byteLength() };
}

} // v8