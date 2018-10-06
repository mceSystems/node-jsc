/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#include <v8.h>
#include "StackTrace.h"
#include "JSCStackTrace.h"

#include <JavaScriptCore/JSObject.h>

namespace v8 { namespace jscshim
{

class Message : public JSC::JSNonFinalObject {
private:
	JSC::WriteBarrier<JSC::JSString> m_message;
	JSC::WriteBarrier<JSC::JSString> m_resourceName;
	JSC::WriteBarrier<JSC::JSString> m_sourceLine;
	JSC::WriteBarrier<jscshim::StackTrace> m_stackTrace;
	JSC::WriteBarrier<JSC::JSString> m_sourceMapUrl;

	int m_scriptId;

	// TODO: Just hold a StackFrame::SourcePositions?
	int m_line;
	int m_startColumn;
	int m_endColumn;
	int m_expressionStart;
	int m_expressionStop;

public:
	using Base = JSC::JSNonFinalObject;

	static Message * create(JSC::ExecState * exec, JSC::Structure * structure, JSC::Exception * exception, bool storeStackTrace)
	{
		JSC::VM& vm = exec->vm();
		Message * message = new (NotNull, JSC::allocateCell<Message>(vm.heap)) Message(vm, structure);
		message->finishCreation(exec, exception, storeStackTrace);
		return message;
	}

	static Message * create(JSC::ExecState * exec, 
							JSC::Structure * structure, 
							JSC::JSValue   thrownValue, 
							JSCStackTrace& stackTrace, 
							bool		   storeStackTrace)
	{
		JSC::VM& vm = exec->vm();
		Message * message = new (NotNull, JSC::allocateCell<Message>(vm.heap)) Message(vm, structure);
		message->finishCreation(exec, thrownValue, stackTrace, storeStackTrace);
		return message;
	}

	static Message * createWithCurrentStackTrace(JSC::ExecState	* exec, 
												 JSC::Structure	* structure, 
												 JSC::JSValue	thrownValue, 
												 bool			storeStackTrace,
												 unsigned int	stackTraceLimit)
	{
		JSC::VM& vm = exec->vm();

		Message * message = new (NotNull, JSC::allocateCell<Message>(vm.heap)) Message(vm, structure);
		JSCStackTrace stackTrace = JSCStackTrace::captureCurrentJSStackTrace(exec, stackTraceLimit);
		message->finishCreation(exec, thrownValue, stackTrace, storeStackTrace);

		return message;
	}

	DECLARE_INFO;

	static JSC::Structure * createStructure(JSC::VM& vm, JSC::JSGlobalObject * globalObject, JSC::JSValue prototype)
	{
		return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
	}

	// Note: v8::Message counts on this to never fail\throw, so changes here will require changes there
	JSC::JSString * message() const { return m_message.get(); }
	JSC::JSString * resourceName() const { return m_resourceName.get(); }
	JSC::JSString * sourceLine() const { return m_sourceLine.get(); }
	JSC::JSString * sourceMapUrl() const { return m_sourceMapUrl.get(); }
	jscshim::StackTrace * stackTrace() const { return m_stackTrace.get(); }
	int scriptId() const { return m_scriptId; }
	int line() const { return m_line; }
	int startColumn() const { return m_startColumn; }
	int endColumn() const { return m_endColumn; }
	int expressionStart() const { return m_expressionStart; }
	int expressionStop() const { return m_expressionStop; }

private:
	Message(JSC::VM& vm, JSC::Structure* structure);

	void finishCreation(JSC::ExecState * exec, JSC::Exception * exception, bool storeStackTrace);
	void finishCreation(JSC::ExecState * exec, JSC::JSValue thrownValue, JSCStackTrace& stackTrace, bool storeStackTrace);

	static void visitChildren(JSC::JSCell *, JSC::SlotVisitor&);

	void GetSourceInformation(JSC::VM& vm, JSCStackTrace& stackTrace);
};


}} // v8::jscshim