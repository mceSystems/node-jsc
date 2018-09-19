/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "V8StackStraceImpl.h"
#include "auto-generated/Runtime.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8_inspector
{

V8StackTraceImpl::V8StackTraceImpl()
{
	// TODO: IMPLEMENT
}

V8StackTraceImpl::~V8StackTraceImpl()
{
	// TODO: IMPLEMENT
}

bool V8StackTraceImpl::isEmpty() const
{
	// TODO: IMPLEMENT
	return true;
}

StringView V8StackTraceImpl::topSourceURL() const
{
	// TODO: IMPLEMENT
	return StringView();
}

int V8StackTraceImpl::topLineNumber() const
{
	// TODO: IMPLEMENT
	return 0;
}

int V8StackTraceImpl::topColumnNumber() const
{
	// TODO: IMPLEMENT
	return 0;
}

StringView V8StackTraceImpl::topScriptId() const
{
	// TODO: IMPLEMENT
	return StringView();
}

StringView V8StackTraceImpl::topFunctionName() const
{
	// TODO: IMPLEMENT
	return StringView();
}

std::unique_ptr<protocol::Runtime::API::StackTrace> V8StackTraceImpl::buildInspectorObject() const
{
	// TODO: IMPLEMENT
	return nullptr;
}

std::unique_ptr<StringBuffer> V8StackTraceImpl::toString() const
{
	// TODO: IMPLEMENT
	return nullptr;
}

std::unique_ptr<V8StackTrace> V8StackTraceImpl::clone()
{
	// TODO: IMPLEMENT
	return nullptr;
}

}