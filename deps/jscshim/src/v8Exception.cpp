/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/Message.h"
#include "shim/StackTrace.h"
#include "shim/helpers.h"
#include "shim/JSCStackTrace.h"

#include <JavaScriptCore/Error.h>
#include <JavaScriptCore/JSCInlines.h>

#define DEFINE_EXCEPTION_FACTORY(exception) \
	Local<Value> Exception::exception(Local<String> message)\
	{\
		JSC::JSString * jscMessage = jscshim::GetJscCellFromV8<JSC::JSString>(*message);\
		return Local<Value>(JSC::create##exception(jscshim::GetCurrentExecState(), \
												   jscMessage->tryGetValue()));\
	}\


namespace v8
{

DEFINE_EXCEPTION_FACTORY(RangeError)
DEFINE_EXCEPTION_FACTORY(ReferenceError)
DEFINE_EXCEPTION_FACTORY(SyntaxError)
DEFINE_EXCEPTION_FACTORY(TypeError)
DEFINE_EXCEPTION_FACTORY(Error)

Local<Message> Exception::CreateMessage(Isolate * isolate, Local<Value> exception)
{
	jscshim::Message * message = jscshim::V8IsolateToJscShimIsolate(isolate)->CreateMessage(exception.val_);
	return Local<Message>(message);
}

Local<StackTrace> Exception::GetStackTrace(Local<Value> exception)
{
	JSC::ExecState * exec = jscshim::GetCurrentExecState();
	JSC::VM& vm = exec->vm();

	jscshim::JSCStackTrace stackTrace = jscshim::JSCStackTrace::getStackTraceForThrownValue(vm, exception.val_);
	if (stackTrace.isEmpty())
	{
		return Local<StackTrace>();
	}

	return Local<StackTrace>(jscshim::StackTrace::create(vm,
														 jscshim::GetGlobalObject(exec)->shimStackTraceStructure(), 
														 exec, 
														 stackTrace));
}

} // v8