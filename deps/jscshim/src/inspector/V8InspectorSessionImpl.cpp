/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "V8InspectorSessionImpl.h"

#include "auto-generated/Runtime.h"
#include "auto-generated/Schema.h"
#include "auto-generated/Debugger.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8_inspector
{

v8::Local<v8::Value> V8InspectorSessionImpl::InspectableImpl::get(v8::Local<v8::Context>)
{
	// TODO: IMPLEMENT
	return v8::Local<v8::Value>();
}

V8InspectorSessionImpl::InspectableImpl::~InspectableImpl()
{
	// TODO: IMPLEMENT
}

V8InspectorSessionImpl::V8InspectorSessionImpl()
{
	// TODO: IMPLEMENT
}

V8InspectorSessionImpl::~V8InspectorSessionImpl()
{
	// TODO: IMPLEMENT
}

void V8InspectorSessionImpl::addInspectedObject(std::unique_ptr<Inspectable>)
{
	// TODO: IMPLEMENT
}

void V8InspectorSessionImpl::dispatchProtocolMessage(const StringView& message)
{
	// TODO: IMPLEMENT
}

std::unique_ptr<StringBuffer> V8InspectorSessionImpl::stateJSON()
{
	// TODO: IMPLEMENT
	return nullptr;
}

std::vector<std::unique_ptr<protocol::Schema::API::Domain>> V8InspectorSessionImpl::supportedDomains()
{
	// TODO: IMPLEMENT
	return std::vector<std::unique_ptr<protocol::Schema::API::Domain>>();
}

void V8InspectorSessionImpl::schedulePauseOnNextStatement(const StringView& breakReason, const StringView& breakDetails)
{
	// TODO: IMPLEMENT
}

void V8InspectorSessionImpl::cancelPauseOnNextStatement()
{
	// TODO: IMPLEMENT
}

void V8InspectorSessionImpl::breakProgram(const StringView& breakReason, const StringView& breakDetails)
{
	// TODO: IMPLEMENT
}

void V8InspectorSessionImpl::setSkipAllPauses(bool)
{
	// TODO: IMPLEMENT
}

void V8InspectorSessionImpl::resume()
{
	// TODO: IMPLEMENT
}

void V8InspectorSessionImpl::stepOver()
{
	// TODO: IMPLEMENT
}

std::vector<std::unique_ptr<protocol::Debugger::API::SearchMatch>>
	V8InspectorSessionImpl::searchInTextByLines(const StringView& text, const StringView& query, bool caseSensitive, bool isRegex)
{
	// TODO: IMPLEMENT
	return std::vector<std::unique_ptr<protocol::Debugger::API::SearchMatch>>();
}

std::unique_ptr<protocol::Runtime::API::RemoteObject> V8InspectorSessionImpl::wrapObject(v8::Local<v8::Context>,
																						 v8::Local<v8::Value>,
																						 const StringView& groupName)
{
	// TODO: IMPLEMENT
	return std::unique_ptr<protocol::Runtime::API::RemoteObject>();
}

bool V8InspectorSessionImpl::unwrapObject(std::unique_ptr<StringBuffer>* error,
										  const StringView& objectId, v8::Local<v8::Value>*,
										  v8::Local<v8::Context>* context,
										  std::unique_ptr<StringBuffer>* objectGroup)
{
	// TODO: IMPLEMENT
	return false;
}

void V8InspectorSessionImpl::releaseObjectGroup(const StringView&)
{
	// TODO: IMPLEMENT
}

}