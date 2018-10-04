/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "Message.h"
#include "helpers.h"

#include <JavaScriptCore/Exception.h>
#include <JavaScriptCore/ErrorInstance.h>
#include <JavaScriptCore/ParseInt.h>
#include <JavaScriptCore/DebuggerPrimitives.h>
#include <JavaScriptCore/Operations.h>
#include <JavaScriptCore/JSCInlines.h>

namespace v8 { namespace jscshim
{

const JSC::ClassInfo Message::s_info = { "Message", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(Message) };

Message::Message(JSC::VM& vm, JSC::Structure * structure) : Base(vm, structure),
	m_scriptId(0),
	m_line(v8::Message::kNoLineNumberInfo),
	m_startColumn(-1),
	m_endColumn(-1),
	m_expressionStart(-1),
	m_expressionStop(-1)
{
}

void Message::finishCreation(JSC::ExecState * exec, JSC::Exception * exception, bool storeStackTrace)
{
	JSC::VM& vm = exec->vm();
	
	// Try to get the exceptions's stack trace
	JSCStackTrace stackTrace = JSCStackTrace::fromExisting(vm, exception->stack());
	if (stackTrace.isEmpty())
	{
		// TODO: Is this "fallback" really needed?
		JSC::ErrorInstance * error = JSC::jsDynamicCast<JSC::ErrorInstance *>(vm, exception->value());
		if (error)
		{
            WTF::Vector<JSC::StackFrame> * jscErrorStackTrace = error->stackTrace();
            if (jscErrorStackTrace)
            {
                stackTrace = JSCStackTrace::fromExisting(vm, *jscErrorStackTrace);
            }
		}
	}

	finishCreation(exec, exception->value(), stackTrace, storeStackTrace);
}

void Message::finishCreation(JSC::ExecState * exec, JSC::JSValue thrownValue, JSCStackTrace& stackTrace, bool storeStackTrace)
{
	JSC::VM& vm = exec->vm();
	Base::finishCreation(vm);

	/* Get the exception (string) message, which is basically the "toString" of the thrown value. 
	 * We have to use SuspendRestoreShimExceptionScope since we (probably) have a pending
	 * exception, which will fail if there's already an exception pending (see Value::ToString's implementation
	 * for more info). 
	 * Note that the "Uncaught " prefix nad "exception" fallback come from v8.
	 * TODO: Cache the "Uncaught " and "Uncaught exception" strings. */
	SuspendRestoreShimExceptionScope shimExceptionScop(jscshim::GetGlobalObject(exec)->isolate());
	JSC::JSString * message = thrownValue.toStringOrNull(exec);
	if (message)
	{
		m_message.set(vm, this, JSC::jsString(exec, JSC::jsString(exec, "Uncaught "), message));
	}
	else
	{
		m_message.set(vm, this, JSC::jsString(exec, "Uncaught exception"));
	}

	if (stackTrace.isEmpty())
	{
		return;
	}

	// Create a stack trace for v8 (if needed)
	if (storeStackTrace)
	{
		m_stackTrace.set(vm, this, jscshim::StackTrace::create(vm, jscshim::GetGlobalObject(exec)->shimStackTraceStructure(), exec, stackTrace));
	}

	GetSourceInformation(vm, stackTrace);
}

void Message::visitChildren(JSC::JSCell * cell, JSC::SlotVisitor& visitor)
{
	Base::visitChildren(cell, visitor);

	Message * thisMessage = JSC::jsCast<Message *>(cell);
	visitor.append(thisMessage->m_message);
	visitor.append(thisMessage->m_resourceName);
	visitor.append(thisMessage->m_sourceLine);
	visitor.append(thisMessage->m_stackTrace);
	visitor.append(thisMessage->m_sourceMapUrl);
}

// General flow here is based on JSC's appendSourceToError (ErrorInstance.cpp)
void Message::GetSourceInformation(JSC::VM& vm, JSCStackTrace& stackTrace)
{
	// Search the stack tracefor the first stack frame with a code block
	JSCStackFrame * stackFrameWithCodeBlock = nullptr;
	for (size_t i = 0; i < stackTrace.size(); i++)
	{
		JSCStackFrame * stackFrame = &stackTrace.at(i);
		if (stackFrame->codeBlock())
		{
			stackFrameWithCodeBlock = stackFrame;
			break;
		}
	}

	if (!stackFrameWithCodeBlock)
	{
		return;
	}

	JSC::SourceProvider * sourceProvider = stackFrameWithCodeBlock->codeBlock()->source();

	/* Store the source\script information
	 * Note that the static_cast is ugly, but in v8 source ids are signed integers, so hopefully
	 * JSC's source id's won't exceed 2^31. */
	m_resourceName.set(vm, this, stackFrameWithCodeBlock->sourceURL());
	m_sourceMapUrl.set(vm, this, JSC::jsString(&vm, sourceProvider->sourceMappingURL()));	
	m_scriptId = static_cast<int>(stackFrameWithCodeBlock->sourceID());
	if (JSC::noSourceID == m_scriptId)
	{
		m_scriptId = v8::Message::kNoScriptIdInfo;
	}

	// Try to get the frame's source "positions" (offsets)
	JSCStackFrame::SourcePositions * sourcePositions = stackFrameWithCodeBlock->getSourcePositions();
	if (!sourcePositions)
	{
		return;
	}
	
	// Store the entire (stack frame's) source line
	unsigned int sourceLineStart = sourcePositions->lineStart.zeroBasedInt();
	unsigned int sourceLineLength = sourcePositions->lineStop.zeroBasedInt() - sourceLineStart;
	WTF::String sourceLine = sourceProvider->source().substring(sourceLineStart, sourceLineLength).toString();
	m_sourceLine.set(vm, this, JSC::jsString(&vm, sourceLine));
	
	// Store the source positions
	m_expressionStart = sourcePositions->expressionStart.zeroBasedInt();
	m_expressionStop = sourcePositions->expressionStop.zeroBasedInt();
	m_line = sourcePositions->line.oneBasedInt();
	m_startColumn = sourcePositions->startColumn.zeroBasedInt();
	m_endColumn = sourcePositions->endColumn.zeroBasedInt();
}

}} // v8::jscshim
