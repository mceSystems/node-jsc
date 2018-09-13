/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include <v8.h>
#include "JSCStackTrace.h"

#include <JavaScriptCore/JSDestructibleObject.h>
#include <JavaScriptCore/JSString.h>

namespace v8 { namespace jscshim
{

class StackFrame : public JSC::JSDestructibleObject {
private:
	/* m_line and m_column are one based
	 * TODO: Use WTF::OrdinalNumber? */
	unsigned int m_line;
	unsigned int m_column;
	int m_scriptId;
	bool m_isEval;
	bool m_isConstructor;
	JSC::WriteBarrier<JSC::JSString> m_functionName;
	JSC::WriteBarrier<JSC::JSString> m_scriptName;

public:
	typedef JSDestructibleObject Base;

	static StackFrame* create(JSC::VM& vm, JSC::Structure* structure, JSCStackFrame& jsFrame)
	{
		StackFrame* cell = new (NotNull, JSC::allocateCell<StackFrame>(vm.heap)) StackFrame(vm, structure);
		cell->finishCreation(vm, jsFrame);
		return cell;
	}

	DECLARE_INFO;

	static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
	{
		return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
	}

	unsigned int line() const { return m_line; }
	unsigned int column() const { return m_column; }
	unsigned int scriptId() const { return m_scriptId; }
	JSC::JSString * functionName() const { return m_functionName.get(); }
	JSC::JSString * scriptName() const { return m_scriptName.get(); }
	bool isEval() const { return m_isEval; }
	bool isConstructor() const { return m_isConstructor; }

private:
	StackFrame(JSC::VM& vm, JSC::Structure* structure) : Base(vm, structure),
		m_line(v8::Message::kNoLineNumberInfo),
		m_column(v8::Message::kNoColumnInfo),
		m_scriptId(v8::Message::kNoScriptIdInfo),
		m_isEval(false),
		m_isConstructor(false)
	{
	}

	void finishCreation(JSC::VM&, JSCStackFrame&);

	static void visitChildren(JSC::JSCell*, JSC::SlotVisitor&);
};

}}