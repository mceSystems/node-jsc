/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include "v8.h"
#include "JSCStackTrace.h"

#include <JavaScriptCore/JSObject.h>

namespace v8 { namespace jscshim
{

class CallSite final : public JSC::JSNonFinalObject {
public:
	enum class Flags
	{
		IsStrict = 1,
		IsEval = 2,
		IsConstructor = 4,
	};

private:
	JSC::WriteBarrier<JSC::Unknown> m_thisValue;
	JSC::WriteBarrier<JSC::Unknown> m_function;
	JSC::WriteBarrier<JSC::Unknown> m_functionName;
	JSC::WriteBarrier<JSC::Unknown> m_sourceURL;
	JSC::JSValue m_lineNumber;
	JSC::JSValue m_columnNumber;
	unsigned int m_flags;

public:
	typedef JSNonFinalObject Base;

	static CallSite * create(JSC::ExecState * exec, JSC::Structure * structure, JSCStackFrame& stackFrame, bool encounteredStrictFrame)
	{
		JSC::VM& vm = exec->vm();
		CallSite * callSite = new (NotNull, JSC::allocateCell<CallSite>(vm.heap)) CallSite(vm, structure);
		callSite->finishCreation(exec, stackFrame, encounteredStrictFrame);
		return callSite;
	}

	DECLARE_INFO;

	static JSC::Structure * createStructure(JSC::VM& vm, JSC::JSGlobalObject * globalObject, JSC::JSValue prototype)
	{
		return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
	}

	JSC::JSValue thisValue() const { return m_thisValue.get(); }
	JSC::JSValue function() const { return m_function.get(); }
	JSC::JSValue functionName() const { return m_functionName.get(); }
	JSC::JSValue sourceURL() const { return m_sourceURL.get(); }
	JSC::JSValue lineNumber() const { return m_lineNumber; }
	JSC::JSValue columnNumber() const { return m_columnNumber; }
	bool isEval() const { return m_flags & static_cast<unsigned int>(Flags::IsEval); }
	bool isConstructor() const { return m_flags & static_cast<unsigned int>(Flags::IsConstructor); }
	bool isStrict() const { return m_flags & static_cast<unsigned int>(Flags::IsStrict); }

private:
	/* Note that v8's JSStackFrame::GetLineNumber & JSStackFrame::GetColumnNumber (messages.cc) fallback to -1, 
	 * so we'll do the same */
	CallSite(JSC::VM& vm, JSC::Structure * structure) : Base(vm, structure),
		m_lineNumber(-1),
		m_columnNumber(-1)
	{
	}

	void finishCreation(JSC::ExecState * exec, JSCStackFrame& stackFrame, bool encounteredStrictFrame);

	static void visitChildren(JSC::JSCell *, JSC::SlotVisitor&);
};

}} // v8::jscshim