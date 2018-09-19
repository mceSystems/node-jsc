/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "StackFrame.h"

#include <JavaScriptCore/CodeBlock.h>
#include <JavaScriptCore/DebuggerPrimitives.h>
#include <JavaScriptCore/JSCInlines.h>

namespace v8 { namespace jscshim
{

const JSC::ClassInfo StackFrame::s_info = { "JSCShimStackFrame", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(StackFrame) };

void StackFrame::finishCreation(JSC::VM& vm, JSCStackFrame& frame)
{
	Base::finishCreation(vm);

	m_scriptName.set(vm, this, frame.sourceURL());
	m_functionName.set(vm, this, frame.functionName());

	JSCStackFrame::SourcePositions * sourcePositions = frame.getSourcePositions();
	if (sourcePositions)
	{
		m_line = sourcePositions->line.oneBasedInt();
		m_column = sourcePositions->startColumn.oneBasedInt();
	}

	m_scriptId = frame.sourceID();
	if (JSC::noSourceID == m_scriptId)
	{
		m_scriptId = v8::Message::kNoScriptIdInfo;
	}

	m_isEval = frame.isEval();
	m_isConstructor = frame.isConstructor();
}

void StackFrame::visitChildren(JSC::JSCell* cell, JSC::SlotVisitor& visitor)
{
	Base::visitChildren(cell, visitor);

	StackFrame * thisObject = JSC::jsCast<StackFrame *>(cell);
	visitor.append(thisObject->m_functionName);
	visitor.append(thisObject->m_scriptName);
}

}} // v8::jscshim