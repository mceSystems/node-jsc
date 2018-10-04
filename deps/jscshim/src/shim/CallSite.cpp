/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "CallSite.h"

#include "helpers.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8 { namespace jscshim
{

const JSC::ClassInfo CallSite::s_info = { "CallSite", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(CallSite) };

void CallSite::finishCreation(JSC::ExecState * exec, JSCStackFrame& stackFrame, bool encounteredStrictFrame)
{
    JSC::VM& vm = exec->vm();
	Base::finishCreation(vm);

    /* From v8's "Stack Trace API" (https://github.com/v8/v8/wiki/Stack-Trace-API): 
	 * "To maintain restrictions imposed on strict mode functions, frames that have a 
	 * strict mode function and all frames below (its caller etc.) are not allow to access 
	 * their receiver and function objects. For those frames, getFunction() and getThis() 
	 * will return undefined.".
     * Thus, if we've already encountered a strict frame, we'll treat our frame as strict too. */
    bool isStrictFrame = encounteredStrictFrame;
    if (!isStrictFrame)
    {
        JSC::CodeBlock * codeBlock = stackFrame.codeBlock();
        if (codeBlock)
        {
            isStrictFrame = codeBlock->isStrictMode();
        }
    }

    // Initialize "this" and "function" (and set the "IsStrict" flag if needed)
    if (isStrictFrame)
    {
        m_thisValue.set(vm, this, JSC::jsUndefined());
        m_function.set(vm, this, JSC::jsUndefined());
        m_flags |= static_cast<unsigned int>(Flags::IsStrict);
    }
    else
    {
        JSC::ExecState * callFrame = stackFrame.callFrame();
        if (callFrame)
        {
            // We know that we're not in strict mode
            m_thisValue.set(vm, this, callFrame->thisValue().toThis(exec, JSC::ECMAMode::NotStrictMode));
        }
        else
        {
            m_thisValue.set(vm, this, JSC::jsUndefined());
        }

        m_function.set(vm, this, stackFrame.callee());
    }
    
    m_functionName.set(vm, this, stackFrame.functionName());
    m_sourceURL.set(vm, this, stackFrame.sourceURL());

    const auto * sourcePositions = stackFrame.getSourcePositions();
    if (sourcePositions)
    {
        m_lineNumber = sourcePositions->line.oneBasedInt();
        m_columnNumber = sourcePositions->startColumn.oneBasedInt();
    }

    if (stackFrame.isEval()) 
    {
        m_flags |= static_cast<unsigned int>(Flags::IsEval);
    }
    if (stackFrame.isConstructor()) 
    {
        m_flags |= static_cast<unsigned int>(Flags::IsConstructor);
    }
}

void CallSite::visitChildren(JSC::JSCell * cell, JSC::SlotVisitor& visitor)
{
	Base::visitChildren(cell, visitor);

	CallSite * thisCallSite = JSC::jsCast<CallSite *>(cell);
	visitor.append(thisCallSite->m_thisValue);
    visitor.append(thisCallSite->m_function);
    visitor.append(thisCallSite->m_functionName);
    visitor.append(thisCallSite->m_sourceURL);
}

}} // v8::jscshim