/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8
{

MaybeLocal<Object> ValueDeserializer::Delegate::ReadHostObject(Isolate* isolate)
{
	// TODO: IMPLEMENT
	return Local<Object>();
}

MaybeLocal<SharedArrayBuffer> ValueDeserializer::Delegate::GetSharedArrayBufferFromId(Isolate* isolate, uint32_t clone_id)
{
	// TODO: IMPLEMENT
	return Local<SharedArrayBuffer>();
}

ValueDeserializer::ValueDeserializer(Isolate* isolate,
									 const uint8_t* data,
									 size_t size,
									 Delegate* delegate)
{
	// TODO: IMPLEMENT
}

ValueDeserializer::~ValueDeserializer()
{
	// TODO: IMPLEMENT
	// From Chakra: "Intentionally left empty to suppress warning C4722."	
}

Maybe<bool> ValueDeserializer::ReadHeader(Local<Context> context)
{
	// TODO: IMPLEMENT
	return Nothing<bool>();
}

MaybeLocal<Value> ValueDeserializer::ReadValue(Local<Context> context)
{
	// TODO: IMPLEMENT
	return Local<Value>();
}

void ValueDeserializer::TransferArrayBuffer(uint32_t transfer_id, Local<ArrayBuffer> array_buffer) {
	// TODO: IMPLEMENT
}

void ValueDeserializer::TransferSharedArrayBuffer(uint32_t id, Local<SharedArrayBuffer> shared_array_buffer)
{
	// TODO: IMPLEMENT
}

uint32_t ValueDeserializer::GetWireFormatVersion() const
{
	// TODO: IMPLEMENT
	return 0;
}

bool ValueDeserializer::ReadUint32(uint32_t* value)
{
	// TODO: IMPLEMENT
	return false;
}

bool ValueDeserializer::ReadUint64(uint64_t* value)
{
	// TODO: IMPLEMENT
	return false;
}

bool ValueDeserializer::ReadDouble(double* value)
{
	// TODO: IMPLEMENT
	return false;
}

bool ValueDeserializer::ReadRawBytes(size_t length, const void** data)
{
	// TODO: IMPLEMENT
	return false;
}

} // v8