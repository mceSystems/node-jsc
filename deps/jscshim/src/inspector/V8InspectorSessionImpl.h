/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "v8-inspector.h"

namespace v8_inspector
{

class V8InspectorSessionImpl : public V8InspectorSession
{
public:
	V8InspectorSessionImpl();
	virtual ~V8InspectorSessionImpl() override;

	// Cross-context inspectable values (DOM nodes in different worlds, etc.).
	class InspectableImpl : public Inspectable {
	public:
		InspectableImpl();
		virtual ~InspectableImpl() override;

		virtual v8::Local<v8::Value> get(v8::Local<v8::Context>) override;
	};
	virtual void addInspectedObject(std::unique_ptr<Inspectable>) override;

	// Dispatching protocol messages.
	virtual void dispatchProtocolMessage(const StringView& message) override;
	virtual std::unique_ptr<StringBuffer> stateJSON() override;
	virtual std::vector<std::unique_ptr<protocol::Schema::API::Domain>> supportedDomains() override;

	// Debugger actions.
	virtual void schedulePauseOnNextStatement(const StringView& breakReason, const StringView& breakDetails) override;
	virtual void cancelPauseOnNextStatement() override;
	virtual void breakProgram(const StringView& breakReason, const StringView& breakDetails) override;
	virtual void setSkipAllPauses(bool) override;
	virtual void resume() override;
	virtual void stepOver() override;
	virtual std::vector<std::unique_ptr<protocol::Debugger::API::SearchMatch>>
		searchInTextByLines(const StringView& text, const StringView& query, bool caseSensitive, bool isRegex) override;

	// Remote objects.
	virtual std::unique_ptr<protocol::Runtime::API::RemoteObject> wrapObject(v8::Local<v8::Context>,
																			 v8::Local<v8::Value>,
																			 const StringView& groupName) override;
	virtual bool unwrapObject(std::unique_ptr<StringBuffer>* error,
							  const StringView& objectId, v8::Local<v8::Value>*,
							  v8::Local<v8::Context>* context,
							  std::unique_ptr<StringBuffer>* objectGroup) override;
	virtual void releaseObjectGroup(const StringView&) override;
};

}