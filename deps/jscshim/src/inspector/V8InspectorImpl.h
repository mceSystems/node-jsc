/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "v8-inspector.h"

namespace v8_inspector 
{

class V8InspectorImpl : public V8Inspector
{
public:
	V8InspectorImpl(v8::Isolate * isolate, V8InspectorClient * client);
	~V8InspectorImpl() override;

	// Contexts instrumentation
	virtual void contextCreated(const V8ContextInfo&) override;
	virtual void contextDestroyed(v8::Local<v8::Context>) override;
	virtual void resetContextGroup(int contextGroupId) override;

	// Various instrumentation
	virtual void idleStarted() override;
	virtual void idleFinished() override;

	// Async stack traces instrumentation
	virtual void asyncTaskScheduled(const StringView& taskName, void* task, bool recurring) override;
	virtual void asyncTaskCanceled(void* task) override;
	virtual void asyncTaskStarted(void* task) override;
	virtual void asyncTaskFinished(void* task) override;
	virtual void allAsyncTasksCanceled() override;

	// Exceptions instrumentation
	virtual unsigned exceptionThrown(v8::Local<v8::Context>, 
									 const StringView& message,
									 v8::Local<v8::Value> exception, 
									 const StringView& detailedMessage,
									 const StringView& url, 
									 unsigned lineNumber, 
									 unsigned columnNumber,
									 std::unique_ptr<V8StackTrace>, 
									 int scriptId) override;
	virtual void exceptionRevoked(v8::Local<v8::Context>, unsigned exceptionId, const StringView& message) override;

	// Connection
	virtual std::unique_ptr<V8InspectorSession> connect(int contextGroupId, Channel*, const StringView& state) override;

	// API methods.
	virtual std::unique_ptr<V8StackTrace> createStackTrace(v8::Local<v8::StackTrace>) override;
	virtual std::unique_ptr<V8StackTrace> captureStackTrace(bool fullStack) override;
};

}