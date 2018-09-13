/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "v8-inspector.h"

namespace v8_inspector
{

class V8StackTraceImpl : public V8StackTrace
{
public:
	V8StackTraceImpl();
	virtual ~V8StackTraceImpl() override;
		
	virtual bool isEmpty() const override;
	virtual StringView topSourceURL() const override;
	virtual int topLineNumber() const override;
	virtual int topColumnNumber() const override;
	virtual StringView topScriptId() const override;
	virtual StringView topFunctionName() const override;

	virtual std::unique_ptr<protocol::Runtime::API::StackTrace> buildInspectorObject() const override;
	virtual std::unique_ptr<StringBuffer> toString() const override;

	virtual std::unique_ptr<V8StackTrace> clone() override;
};

}