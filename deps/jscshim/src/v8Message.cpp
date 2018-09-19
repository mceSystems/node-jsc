/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"
#include "shim/Message.h"

#include "shim/helpers.h"

#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<jscshim::Message>(this)

namespace v8
{

const int Message::kNoLineNumberInfo;
const int Message::kNoColumnInfo;
const int Message::kNoScriptIdInfo;

Local<String> Message::Get() const
{
	return Local<String>(GET_JSC_THIS()->message());
}

Local<String> Message::GetSourceLine() const
{
	return Local<String>(GET_JSC_THIS()->sourceLine());
}

MaybeLocal<String> Message::GetSourceLine(Local<Context> context) const
{
	return Local<String>(GET_JSC_THIS()->sourceLine());
}

ScriptOrigin Message::GetScriptOrigin() const
{
	Isolate * isolate = Isolate::GetCurrent();
	jscshim::Message * jscMessage = GET_JSC_THIS();

	/* According to v8's ErrorWithMissingScriptInfo unit test, API users expect "undefined" 
	 * and not an empty string when there's no resource name */
	JSC::JSString * originalResourceName = jscMessage->resourceName();
	JSC::JSValue originResourceName = (0 == originalResourceName->length()) ? JSC::jsUndefined() : originalResourceName;

	// TODO: The rest of ScriptOrigin's arguments
	return ScriptOrigin(Local<Value>(originResourceName),
						Integer::New(isolate, jscMessage->line()),
						Integer::New(isolate, jscMessage->startColumn()),
						v8::False(isolate), // TODO
						Integer::New(isolate, jscMessage->scriptId()),
						Local<Value>(jscMessage->sourceMapUrl()));
}

Local<Value> Message::GetScriptResourceName() const
{
	return Local<Value>(GET_JSC_THIS()->resourceName());
}

Local<StackTrace> Message::GetStackTrace() const
{
	return Local<StackTrace>(GET_JSC_THIS()->stackTrace());
}

int Message::GetLineNumber() const
{
	return GET_JSC_THIS()->line();
}

Maybe<int> Message::GetLineNumber(Local<Context> context) const
{
	return Just(GET_JSC_THIS()->line());
}

int Message::GetStartPosition() const
{
	return GET_JSC_THIS()->expressionStart();
}

int Message::GetEndPosition() const
{
	return GET_JSC_THIS()->expressionStop();
}

int Message::GetStartColumn() const
{
	return GET_JSC_THIS()->startColumn();
}

Maybe<int> Message::GetStartColumn(Local<Context> context) const
{
	return Just(GET_JSC_THIS()->startColumn());
}

int Message::GetEndColumn() const
{
	return GET_JSC_THIS()->endColumn();
}

Maybe<int> Message::GetEndColumn(Local<Context> context) const
{
	return Just(GET_JSC_THIS()->endColumn());
}

} // v8