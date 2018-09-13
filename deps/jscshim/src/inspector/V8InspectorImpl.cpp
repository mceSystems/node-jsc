/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "V8InspectorImpl.h"
#include "V8InspectorSessionImpl.h"
#include "V8StackStraceImpl.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8_inspector
{

V8InspectorImpl::V8InspectorImpl(v8::Isolate * isolate, V8InspectorClient * client)
{
	// TODO: IMPLEMENT
}

V8InspectorImpl::~V8InspectorImpl()
{
	// TODO: IMPLEMENT
}

void V8InspectorImpl::contextCreated(const V8ContextInfo&)
{
	// TODO: IMPLEMENT
}

void V8InspectorImpl::contextDestroyed(v8::Local<v8::Context>)
{
	// TODO: IMPLEMENT
}

void V8InspectorImpl::resetContextGroup(int contextGroupId)
{
	// TODO: IMPLEMENT
}

void V8InspectorImpl::idleStarted()
{
	// TODO: IMPLEMENT
}

void V8InspectorImpl::idleFinished()
{
	// TODO: IMPLEMENT
}

void V8InspectorImpl::asyncTaskScheduled(const StringView& taskName, void* task, bool recurring)
{
	// TODO: IMPLEMENT
}

void V8InspectorImpl::asyncTaskCanceled(void* task)
{
	// TODO: IMPLEMENT
}

void V8InspectorImpl::asyncTaskStarted(void* task)
{
	// TODO: IMPLEMENT
}

void V8InspectorImpl::asyncTaskFinished(void* task)
{
	// TODO: IMPLEMENT
}

void V8InspectorImpl::allAsyncTasksCanceled()
{
	// TODO: IMPLEMENT
}

unsigned V8InspectorImpl::exceptionThrown(v8::Local<v8::Context> context,
										  const StringView& message,
										  v8::Local<v8::Value> exception,
										  const StringView&	detailedMessage,
										  const StringView& url,
										  unsigned lineNumber,
										  unsigned columnNumber,
										  std::unique_ptr<V8StackTrace> stackTrace,
										  int scriptId)
{
	// TODO: IMPLEMENT
	return 0;
}

void V8InspectorImpl::exceptionRevoked(v8::Local<v8::Context>, unsigned exceptionId, const StringView& message)
{
	// TODO: IMPLEMENT
}

std::unique_ptr<V8InspectorSession> V8InspectorImpl::connect(int contextGroupId, Channel*, const StringView& state)
{
	// TODO: IMPLEMENT
	return std::unique_ptr<V8InspectorSessionImpl>();
}

std::unique_ptr<V8StackTrace> V8InspectorImpl::createStackTrace(v8::Local<v8::StackTrace>)
{
	// TODO: IMPLEMENT
	return std::unique_ptr<V8StackTraceImpl>();
}

std::unique_ptr<V8StackTrace> V8InspectorImpl::captureStackTrace(bool fullStack)
{
	// TODO: IMPLEMENT
	return std::unique_ptr<V8StackTraceImpl>();
}

}