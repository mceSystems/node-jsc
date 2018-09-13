/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include <JavaScriptCore/JSArrayBuffer.h>
#include <JavaScriptCore/Error.h>

#define GET_JSC_THIS_ARRAY_BUFFER() v8::jscshim::GetJscCellFromV8<JSC::JSArrayBuffer>(this)->impl()

namespace v8 { namespace jscshim
{

inline JSC::JSArrayBuffer * CreateJscArrayBuffer(v8::Isolate * isolate, size_t byte_length, JSC::ArrayBufferSharingMode sharingMode)
{
	JSC::VM& vm = V8IsolateToJscShimIsolate(isolate)->VM();
	JSC::ExecState * exec = GetExecStateForV8Isolate(isolate);
	DECLARE_SHIM_EXCEPTION_SCOPE(isolate);

	auto buffer = JSC::ArrayBuffer::tryCreate(byte_length, 1);
	if (!buffer)
	{
		SHIM_THROW_EXCEPTION(JSC::createOutOfMemoryError(exec));
		return nullptr;
	}

	if (JSC::ArrayBufferSharingMode::Shared == sharingMode)
	{
		buffer->makeShared();
	}

	JSC::Structure * arrayBufferStructure = exec->lexicalGlobalObject()->arrayBufferStructure(sharingMode);
	return JSC::JSArrayBuffer::create(vm, arrayBufferStructure, WTFMove(buffer));
}

inline WTF::Ref<JSC::ArrayBuffer> CreateArrayBufferFromBytes(v8::Isolate			 * isolate,
															 void					 * data,
															 size_t					 byteLength,
															 ArrayBufferCreationMode mode)
{
	if (ArrayBufferCreationMode::kExternalized == mode)
	{
		/* TODO: makeApiUserControlledBuffer will override the destructor, so we're setting the destructor twice.
		 * We should optimize this. */
		WTF::Ref<JSC::ArrayBuffer> newArrayBuffer = JSC::ArrayBuffer::createFromBytes(data, byteLength, [](void*) {});
		newArrayBuffer->makeApiUserControlledBuffer();
		return WTFMove(newArrayBuffer);
	}

	return JSC::ArrayBuffer::createFromBytes(data, byteLength, [isolate, byteLength](void * p) {
		V8IsolateToJscShimIsolate(isolate)->ArrayBufferAllocator()->Free(p, byteLength);
	});
}

inline JSC::JSArrayBuffer * CreateJscArrayBuffer(v8::Isolate				 * isolate,
												 void						 * data,
												 size_t						 byteLength,
												 ArrayBufferCreationMode	 mode, 
												 JSC::ArrayBufferSharingMode sharingMode)
{
	JSC::VM& vm = V8IsolateToJscShimIsolate(isolate)->VM();
	JSC::ExecState * exec = GetExecStateForV8Isolate(isolate);

	JSC::Structure * arrayBufferStructure = exec->lexicalGlobalObject()->arrayBufferStructure(sharingMode);
	auto buffer = CreateArrayBufferFromBytes(isolate, data, byteLength, mode);
	if (JSC::ArrayBufferSharingMode::Shared == sharingMode)
	{
		buffer->makeShared();
	}

	return JSC::JSArrayBuffer::create(vm, arrayBufferStructure, WTFMove(buffer));
}

}} // v8::jscshim