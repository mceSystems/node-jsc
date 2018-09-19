/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/FunctionTemplate.h"
#include "shim/Function.h"
#include "shim/GlobalObject.h"
#include "shim/helpers.h"

#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<jscshim::FunctionTemplate>(this)

namespace v8
{

// TODO: ConstructorBehavior
// TODO: SideEffectType
Local<FunctionTemplate> FunctionTemplate::New(Isolate * isolate, 
											  FunctionCallback callback, 
											  Local<Value> data, 
											  Local<Signature> signature,
											  int length,
											  ConstructorBehavior behavior,
											  SideEffectType side_effect_type)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Isolate(isolate);
	JSC::VM& vm = exec->vm();

	jscshim::FunctionTemplate * signatureFunctionTemplate = nullptr;
	if (signature.val_)
	{
		signatureFunctionTemplate = JSC::jsDynamicCast<jscshim::FunctionTemplate *>(vm, signature.val_);
		ASSERT(signatureFunctionTemplate);
	}
	
	jscshim::FunctionTemplate * newTemplate = jscshim::FunctionTemplate::create(exec->vm(), 
																				jscshim::GetGlobalObject(exec)->shimFunctionTemplateStructure(),
																				exec,
																				callback,
																				length,
																				jscshim::GetValue(*data), 
																				signatureFunctionTemplate);

	return Local<FunctionTemplate>::New(JSC::JSValue(newTemplate));
}

Local<Function> FunctionTemplate::GetFunction()
{
	jscshim::FunctionTemplate * thisTemplate = GET_JSC_THIS();
	
	// TODO: Should we just call jscshim::GetCurrentExecState?
	JSC::ExecState * exec = thisTemplate->globalObject()->globalExec();

	// TODO: Ask callers for their Isolate
	DECLARE_SHIM_EXCEPTION_SCOPE(exec);
	jscshim::Function * newFunction = thisTemplate->makeNewFunctionInstance(exec);
	return Local<Function>::New(JSC::JSValue(newFunction));
}

MaybeLocal<Function> FunctionTemplate::GetFunction(Local<Context> context)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context);
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);
	
	jscshim::Function * newFunction = GET_JSC_THIS()->makeNewFunctionInstance(exec);
	return Local<Function>::New(JSC::JSValue(newFunction));
}

void FunctionTemplate::Inherit(Local<FunctionTemplate> parent)
{
	GET_JSC_THIS()->setParentTemplate(jscshim::GetCurrentVM(), 
									  jscshim::GetJscCellFromV8<jscshim::FunctionTemplate>(*parent));
}

Local<ObjectTemplate> FunctionTemplate::PrototypeTemplate()
{
	jscshim::FunctionTemplate * thisTemplate = GET_JSC_THIS();

	// TODO: Should we just call jscshim::GetCurrentExecState?
	JSC::ExecState * exec = thisTemplate->globalObject()->globalExec();
	
	return Local<ObjectTemplate>::New(JSC::JSValue(thisTemplate->getPrototypeTemplate(exec)));
}

void FunctionTemplate::SetPrototypeProviderTemplate(Local<FunctionTemplate> prototype_provider)
{
	GET_JSC_THIS()->setPrototypeProviderTemplate(jscshim::GetCurrentExecState(), 
												 jscshim::GetJscCellFromV8<jscshim::FunctionTemplate>(*prototype_provider));
}

void FunctionTemplate::SetClassName(Local<String> name)
{
	GET_JSC_THIS()->setTemplateClassName(jscshim::GetCurrentVM(), 
										 v8::jscshim::GetJscCellFromV8<JSC::JSString>(*name));
}

void FunctionTemplate::SetCallHandler(FunctionCallback callback, Local<Value> data)
{
	GET_JSC_THIS()->setCallHandler(callback, data.val_);
}

void FunctionTemplate::SetLength(int length)
{
	GET_JSC_THIS()->setLength(length);
}

Local<ObjectTemplate> FunctionTemplate::InstanceTemplate()
{
	jscshim::FunctionTemplate * thisTemplate = GET_JSC_THIS();

	// TODO: Should we just call jscshim::GetCurrentExecState?
	JSC::ExecState * exec = thisTemplate->globalObject()->globalExec();

	return Local<ObjectTemplate>::New(JSC::JSValue(thisTemplate->getInstnaceTemplate(exec)));
}

void FunctionTemplate::SetHiddenPrototype(bool value)
{
	GET_JSC_THIS()->setHiddenPrototype(value);
}

void FunctionTemplate::ReadOnlyPrototype()
{
	GET_JSC_THIS()->setReadOnlyPrototype(true);
}

void FunctionTemplate::RemovePrototype()
{
	GET_JSC_THIS()->setRemovePrototype(true);
}

bool FunctionTemplate::HasInstance(Local<Value> object)
{
	return GET_JSC_THIS()->isTemplateFor(object.val_);
}

} // v8