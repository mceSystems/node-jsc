/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "StackFrame.h"
#include "JSCStackTrace.h"

#include <JavaScriptCore/JSDestructibleObject.h>
#include <JavaScriptCore/StackFrame.h>
#include <JavaScriptCore/JSArray.h>
#include <JavaScriptCore/Interpreter.h>
#include <JavaScriptCore/Exception.h>

namespace v8 { namespace jscshim
{

class StackTrace final : public JSC::JSDestructibleObject {
private:
	WTF::Vector<JSC::WriteBarrier<StackFrame>> m_frames;

public:
	typedef JSDestructibleObject Base;

	static StackTrace * create(JSC::VM& vm, JSC::Structure * structure, JSC::ExecState * exec, JSCStackTrace& stackTrace)
	{
		StackTrace* cell = new (NotNull, JSC::allocateCell<StackFrame>(vm.heap)) StackTrace(vm, structure);
		cell->finishCreation(vm, exec, stackTrace);
		return cell;
	}

	static StackTrace * create(JSC::VM& vm, JSC::Structure * structure, JSC::ExecState * exec, JSC::Exception * exception)
	{
		/* TODO: Copy JSC's createScriptCallStackFromException "Fallback to getting at least the line and sourceURL from the 
		 * exception object if it has values and the exceptionStack doesn't." */
		 JSCStackTrace jscStackTrace = JSCStackTrace::fromExisting(vm, exception->stack());
		return create(vm, structure, exec, jscStackTrace);
	}

	static StackTrace * createWithCurrent(JSC::VM& vm, JSC::Structure * structure, JSC::ExecState * exec, size_t frameLimit)
	{
		StackTrace* cell = new (NotNull, JSC::allocateCell<StackFrame>(vm.heap)) StackTrace(vm, structure);
		cell->Base::finishCreation(vm);

		if (frameLimit)
		{
			cell->captureCurrentStackTrace(vm, exec, frameLimit);
		}

		return cell;
	}

	DECLARE_INFO;

	static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
	{
		return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
	}

	StackFrame * getFrame(unsigned int index) const 
	{ 
		return m_frames[index].get();
	}

	unsigned int frameCount() const
	{
		return m_frames.size();
	}

private:
	StackTrace(JSC::VM& vm, JSC::Structure* structure) : Base(vm, structure)
	{
	}

	void finishCreation(JSC::VM& vm, JSC::ExecState * exec, JSCStackTrace& stackTrace);

	static void visitChildren(JSC::JSCell*, JSC::SlotVisitor&);
	static void destroy(JSC::JSCell*);

	// TODO: Rename as an overload to finishCreation?
	void captureCurrentStackTrace(JSC::VM& vm, JSC::ExecState * exec, size_t frameLimit);
};

}}