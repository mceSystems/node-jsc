/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "StackTrace.h"

#include "helpers.h"

#include <JavaScriptCore/GCDeferralContext.h>
#include <JavaScriptCore/JSCInlines.h>

namespace v8 { namespace jscshim
{

const JSC::ClassInfo StackTrace::s_info = { "JSCShimStackTrace", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(StackTrace) };

void StackTrace::finishCreation(JSC::VM& vm, JSC::ExecState * exec, JSCStackTrace& stackTrace)
{
	Base::finishCreation(vm);

	GlobalObject * global = static_cast<GlobalObject *>(exec->lexicalGlobalObject());
	JSC::Structure * stackFrameStructure = global->shimStackFrameStructure();

	size_t frameCount = stackTrace.size();
    {
        auto locker = holdLock(cellLock());
        m_frames.reserveInitialCapacity(frameCount);
        for (unsigned int i = 0; i < frameCount; ++i)
        {
            JSCStackFrame& frame = stackTrace.at(i);
            
            /* v8 doesn't include native (non js and non wasm) frames in the stack trace
             * This is based on what JSC::StackVisitor::Frame::isNativeFrame does. */
            if (frame.codeBlock() || frame.isWasmFrame())
            {
                m_frames.constructAndAppend(vm, this, StackFrame::create(vm, stackFrameStructure, frame));
            }
        }
    }
}

void StackTrace::visitChildren(JSC::JSCell* cell, JSC::SlotVisitor& visitor)
{
	Base::visitChildren(cell, visitor);

    {
        StackTrace * thisObject = JSC::jsCast<StackTrace *>(cell);
        auto locker = holdLock(thisObject->cellLock());
        for (auto& frame : thisObject->m_frames)
        {
            // Note: Using "append" causes a compilation error
            visitor.append(frame);
        }
    }
}

/* Note that at first I've allocated jscshim::StackFrame instances while walking the JSC stack. This failed 
 * because of an ASSERT in JSC's tryAllocateCellHelper, because of the JSC::DisallowGC used (which was there 
 * in JSC::Interpreter::getStackTrace). One possible solution is to use a JSC::GCDeferralContext, but besides needing
 * to understand it's implications, it also requires adding GCDeferralContextInlines.h to JSC's forwarding headers.
 * For now we'll just collect all stack frames first, and than create our jscshim::StackFrame instances.
 * TODO: Should we refactor this to use JSC::GCDeferralContext? */
void StackTrace::captureCurrentStackTrace(JSC::VM& vm, JSC::ExecState * exec, size_t frameLimit)
{
	JSCStackTrace stackFrames = JSCStackTrace::captureCurrentJSStackTrace(exec, frameLimit);
	size_t framesCount = stackFrames.size();

	// Create our StackFrames for each JSC frame
    {
        auto locker = holdLock(cellLock());
        m_frames.reserveInitialCapacity(framesCount);
        JSC::Structure * stackFrameStructure = jscshim::GetGlobalObject(exec)->shimStackFrameStructure();
        for (size_t i = 0; i < framesCount; i++)
        {
            m_frames.constructAndAppend(vm, this, StackFrame::create(vm, stackFrameStructure, stackFrames.at(i)));
        }
    }
}

}} // v8::jscshim