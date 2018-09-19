/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"
#include "libplatform/libplatform.h"

namespace v8 { namespace platform
{

v8::Platform * CreateDefaultPlatform(int thread_pool_size, IdleTaskSupport idle_task_support)
{
	// TODO: IMPLEMENT
	return nullptr;
}

bool PumpMessageLoop(v8::Platform* platform, v8::Isolate* isolate, MessageLoopBehavior behavior)
{
	// TODO: IMPLEMENT
	return false;
}

void SetTracingController(Platform* platform, platform::tracing::TracingController* tracing_controller)
{
	// TODO: IMPLEMENT
}

}} // v8::platform