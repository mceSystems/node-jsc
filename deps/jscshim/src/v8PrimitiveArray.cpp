/**
 * This source code is licensed under the terms found in the LICENSE file in
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"
#include "shim/Script.h"

namespace v8
{

Local<PrimitiveArray> PrimitiveArray::New(Isolate* isolate, int length)
{
	// TODO: IMPLEMENT
	return Local<PrimitiveArray>();
}

int PrimitiveArray::Length() const
{
	// TODO: IMPLEMENT
	return 0;
}

void PrimitiveArray::Set(int index, Local<Primitive> item)
{
	// TODO: IMPLEMENT
}

Local<Primitive> PrimitiveArray::Get(int index)
{
	// TODO: IMPLEMENT
	return Local<Primitive>();
}

} // v8


