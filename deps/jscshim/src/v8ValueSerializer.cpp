/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8
{

Maybe<bool> ValueSerializer::Delegate::WriteHostObject(Isolate* isolate, Local<Object> object)
{
	// TODO: IMPLEMENT
	return Nothing<bool>();
}

Maybe<uint32_t> ValueSerializer::Delegate::GetSharedArrayBufferId(Isolate* isolate, 
																  Local<SharedArrayBuffer> shared_array_buffer)
{
	// TODO: IMPLEMENT
	return Nothing<unsigned int>();
}

ValueSerializer::ValueSerializer(Isolate* isolate)
{
	// TODO: IMPLEMENT
}

ValueSerializer::ValueSerializer(Isolate* isolate, Delegate* delegate)
{
	// TODO: IMPLEMENT
}

ValueSerializer::~ValueSerializer()
{
	// TODO: IMPLEMENT
	// From Chakra: "Intentionally left empty to suppress warning C4722."
}

void ValueSerializer::WriteHeader()
{
	// TODO: IMPLEMENT
}

Maybe<bool> ValueSerializer::WriteValue(Local<Context> context, Local<Value> value)
{
	// TODO: IMPLEMENT
	return Nothing<bool>();
}

std::pair<uint8_t*, size_t> ValueSerializer::Release()
{
	// TODO: IMPLEMENT
	return std::pair<uint8_t*, size_t>();
}

void ValueSerializer::TransferArrayBuffer(uint32_t transfer_id, Local<ArrayBuffer> array_buffer)
{
	// TODO: IMPLEMENT
}

void ValueSerializer::SetTreatArrayBufferViewsAsHostObjects(bool mode)
{
	// TODO: IMPLEMENT
}

void ValueSerializer::WriteUint32(uint32_t value)
{
	// TODO: IMPLEMENT
}

void ValueSerializer::WriteUint64(uint64_t value)
{
	// TODO: IMPLEMENT
}

void ValueSerializer::WriteDouble(double value)
{
	// TODO: IMPLEMENT
}

void ValueSerializer::WriteRawBytes(const void* source, size_t length)
{
	// TODO: IMPLEMENT
}

} // v8