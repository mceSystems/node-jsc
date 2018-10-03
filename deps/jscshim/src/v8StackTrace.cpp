/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/StackTrace.h"
#include "shim/helpers.h"

#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS_STACK_TRACE() v8::jscshim::GetJscCellFromV8<jscshim::StackTrace>(this)
#define GET_JSC_THIS_STACK_FRAME() v8::jscshim::GetJscCellFromV8<jscshim::StackFrame>(this)

namespace v8
{

Local<StackFrame> StackTrace::GetFrame(uint32_t index) const
{
	return Local<StackFrame>::New(JSC::JSValue(GET_JSC_THIS_STACK_TRACE()->getFrame(index)));
}

int StackTrace::GetFrameCount() const
{
	return GET_JSC_THIS_STACK_TRACE()->frameCount();
}

// TODO: Handle options?
Local<StackTrace> StackTrace::CurrentStackTrace(Isolate* isolate,
												int frame_limit,
												StackTraceOptions options)
{
	jscshim::Isolate * jscIsolate = jscshim::V8IsolateToJscShimIsolate(isolate);
	JSC::VM& vm = jscIsolate->VM();
	JSC::ExecState * exec = jscIsolate->GetCurrentContext()->v8ContextExec();

	size_t frameCount = static_cast<size_t>(std::max(frame_limit, 0));
	jscshim::StackTrace * v8StackTrace = jscshim::StackTrace::createWithCurrent(vm,
																			    jscshim::GetGlobalObject(exec)->shimStackTraceStructure(), 
																			    exec,
																			    frameCount);

	return Local<StackTrace>::New(JSC::JSValue(v8StackTrace));
}

int StackFrame::GetLineNumber() const
{
	return GET_JSC_THIS_STACK_FRAME()->line();
}

int StackFrame::GetColumn() const
{
	return GET_JSC_THIS_STACK_FRAME()->column();
}

int StackFrame::GetScriptId() const
{
	return GET_JSC_THIS_STACK_FRAME()->scriptId();
}

Local<String> StackFrame::GetScriptName() const
{
	return Local<String>::New(JSC::JSValue(GET_JSC_THIS_STACK_FRAME()->scriptName()));
}

Local<String> StackFrame::GetScriptNameOrSourceURL() const
{
	return Local<String>::New(JSC::JSValue(GET_JSC_THIS_STACK_FRAME()->scriptName()));
}

Local<String> StackFrame::GetFunctionName() const
{
	return Local<String>::New(JSC::JSValue(GET_JSC_THIS_STACK_FRAME()->functionName()));
}

bool StackFrame::IsEval() const
{
	return GET_JSC_THIS_STACK_FRAME()->isEval();
}

bool StackFrame::IsConstructor() const
{
	return GET_JSC_THIS_STACK_FRAME()->isConstructor();
}

} // v8