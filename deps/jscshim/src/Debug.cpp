/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8-debug.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8
{

Local<Context> Debug::GetDebugContext(Isolate* isolate)
{
	// TODO: IMPLEMENT
	return Local<Context>();
}

bool Debug::SetDebugEventListener(Isolate* isolate, EventCallback that, Local<Value> data)
{
	// TODO: IMPLEMENT
	return false;
}

void Debug::DebugBreak(Isolate * isolate)
{
	// TODO: IMPLEMENT
}

} // v8