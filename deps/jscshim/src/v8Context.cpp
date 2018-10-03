/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"
#include "shim/GlobalObject.h"
#include "shim/FunctionTemplate.h"
#include "shim/Object.h"
#include "shim/ObjectTemplate.h"

#include <JavaScriptCore/InitializeThreading.h>
#include <JavaScriptCore/JSCInlines.h>
#include <wtf/Assertions.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<jscshim::GlobalObject>(this)

namespace
{

inline void verifiyEmbedderDataAccess(v8::jscshim::GlobalObject * context, int index, bool canGrow, const char* location)
{
	v8::jscshim::ApiCheck(index >= 0, location, "Negative index");
	v8::jscshim::ApiCheck(canGrow || (index <= context->contextEmbedderDataSize()), location, "Index too large");
}

}

namespace v8
{

Local<Object> Context::Global()
{
	/* Note: JSC proxies the global object, just like v8. Thus, we need
	 * to use globalThis, which points to that proxy, and not just our GlobalObject instance */
	return Local<Object>::New(GET_JSC_THIS()->globalThis());
}

Local<Context> Context::New(Isolate* isolate,
							ExtensionConfiguration* extensions,
							MaybeLocal<ObjectTemplate> global_template,
							MaybeLocal<Value> global_object)
{
	Local<Value> what;
	MaybeLocal<Value> test(what);

	// TODO: IMPLEMENT extensions and global_object parameters
	if (extensions || !global_object.IsEmpty()) {
		WTFReportFatalError(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, "Context::New with externsions/global_object is not supported yet");
	}
	
	// Based on JSGlobalContextCreateInGroup
	JSC::initializeThreading();

	jscshim::Isolate * jscIsolate = jscshim::V8IsolateToJscShimIsolate(isolate);
	JSC::VM& vm = jscIsolate->VM();
	JSC::JSLockHolder locker(vm);
	DECLARE_SHIM_EXCEPTION_SCOPE(jscIsolate);

	/* JSC's JSGlobalContextCreateInGroup always creates the global object with a null prototype, 
	 * then if necessary sets the global object's prototype (globalObject->resetPrototype). This seems
	 * like an unnecessary overhead, as JSC::GlobalObject::init (internally called when creating a new global
	 * object) will call resetPrototype for us. */
	JSC::JSGlobalObject * globalObject = nullptr;
	if (!global_template.IsEmpty())
	{
		jscshim::ObjectTemplate * globalObjectTemplate = jscshim::GetJscCellFromV8<jscshim::ObjectTemplate>(*global_template.ToLocalChecked());
		ASSERT(globalObjectTemplate);

		JSC::ExecState * exec = jscIsolate->GetCurrentGlobalOrDefault()->globalExec();
		JSC::JSObject * globalObjectPrototype = globalObjectTemplate->makeNewObjectInstance(exec, JSC::JSValue());
		SHIM_RETURN_IF_EXCEPTION(Local<Context>());

		globalObject = jscshim::GlobalObject::create(vm,
													 jscshim::GlobalObject::createStructure(vm, globalObjectPrototype),
													 jscIsolate,
													 globalObjectTemplate->internalFieldCount());
	}
	else
	{
		globalObject = jscshim::GlobalObject::create(vm,
													 jscshim::GlobalObject::createStructure(vm, JSC::jsNull()),
													 jscIsolate,
													 0);
	}

	return Local<Context>::New(JSC::JSValue(globalObject));
}

Isolate* Context::GetIsolate()
{
	return reinterpret_cast<Isolate *>(GET_JSC_THIS()->isolate());
}

void Context::SetSecurityToken(Local<Value> token)
{
	// TODO: IMPLEMENT
}

Local<Value> Context::GetSecurityToken()
{
	// TODO: IMPLEMENT
	return Local<Value>();
}

void Context::Enter()
{
	jscshim::GlobalObject * context = GET_JSC_THIS();
	jscshim::Isolate * isolate = context->isolate();

	isolate->EnterContext(context);
	isolate->SetCurrentContext(context, true);
}

void Context::Exit()
{
	jscshim::GlobalObject * context = GET_JSC_THIS();
	jscshim::Isolate * isolate = context->isolate();
	
	jscshim::ApiCheck(isolate->GetEnteredContext() == context,
					  "v8::Context::Exit()",
					  "Cannot exit non-entered context");

	isolate->LeaveEnteredContext();
	isolate->RestoreCurrentContextFromSaved();
}

Local<Value> Context::GetEmbedderData(int index)
{
	jscshim::GlobalObject * context = GET_JSC_THIS();
	verifiyEmbedderDataAccess(context, index, false, "v8::Context::SetEmbedderData()");

	return Local<Value>::New(context->getValueFromEmbedderData(index));
}

void Context::SetEmbedderData(int index, Local<Value> value)
{
	jscshim::GlobalObject * context = GET_JSC_THIS();

	verifiyEmbedderDataAccess(context, index, true, "v8::Context::SetEmbedderData()");
	context->setValueInEmbedderData(index, value.val_);
}

void * Context::GetAlignedPointerFromEmbedderData(int index)
{
	jscshim::GlobalObject * context = GET_JSC_THIS();

	verifiyEmbedderDataAccess(context, index, false, "v8::Context::GetAlignedPointerFromEmbedderData()");
	return context->getAlignedPointerFromEmbedderData(index);
}

void Context::SetAlignedPointerInEmbedderData(int index, void* value)
{
	jscshim::GlobalObject * context = GET_JSC_THIS();

	verifiyEmbedderDataAccess(context, index, true, "v8::Context::SetAlignedPointerInEmbedderData()");
	context->setAlignedPointerInEmbedderData(index, value);
}

} // v8
