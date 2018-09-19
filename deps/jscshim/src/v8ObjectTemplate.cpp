/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/GlobalObject.h"
#include "shim/ObjectTemplate.h"
#include "shim/FunctionTemplate.h"
#include "shim/Object.h"
#include "shim/helpers.h"
#include "base/logging.h"

#include <JavaScriptCore/CatchScope.h>
#include <JavaScriptCore/JSCInlines.h>

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<jscshim::ObjectTemplate>(this)

namespace v8
{

Local<ObjectTemplate> ObjectTemplate::New(Isolate * isolate, Local<FunctionTemplate> constructor)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Isolate(isolate);
	jscshim::GlobalObject * global = jscshim::GetGlobalObject(exec);
	JSC::VM& vm = global->vm();

	jscshim::ObjectTemplate * newTemplate = jscshim::ObjectTemplate::create(vm, 
																			global->shimObjectTemplateStructure(), 
																			exec, 
																			jscshim::GetJscCellFromV8<jscshim::FunctionTemplate>(*constructor));

	return Local<ObjectTemplate>(JSC::JSValue(newTemplate));
}

MaybeLocal<Object> ObjectTemplate::NewInstance(Local<Context> context)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context);
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);

	jscshim::Object * newObject = GET_JSC_THIS()->makeNewObjectInstance(exec, JSC::JSValue());
	SHIM_RETURN_IF_EXCEPTION(Local<Object>());

	return Local<Object>(JSC::JSValue(newObject));
}

void ObjectTemplate::SetAccessor(Local<String> name,
								 AccessorGetterCallback getter, 
								 AccessorSetterCallback setter,
								 Local<Value> data, 
								 AccessControl settings, 
								 PropertyAttribute attribute,
								 Local<AccessorSignature> signature)
{
	GET_JSC_THIS()->SetAccessor(jscshim::GetCurrentExecState(),
								name.val_.asCell(),
								reinterpret_cast<AccessorNameGetterCallback>(getter),
								reinterpret_cast<AccessorNameSetterCallback>(setter),
								data.val_,
								jscshim::GetJscCellFromV8<jscshim::FunctionTemplate>(*signature),
								jscshim::v8PropertyAttributesToJscAttributes(attribute),
								false);
}

void ObjectTemplate::SetAccessor(Local<Name> name,
								 AccessorNameGetterCallback getter,
								 AccessorNameSetterCallback setter,
								 Local<Value> data,
								 AccessControl settings,
								 PropertyAttribute attribute,
								 Local<AccessorSignature> signature)
{
	GET_JSC_THIS()->SetAccessor(jscshim::GetCurrentExecState(),
								name.val_.asCell(),
								getter,
								setter,
								data.val_,
								jscshim::GetJscCellFromV8<jscshim::FunctionTemplate>(*signature),
								jscshim::v8PropertyAttributesToJscAttributes(attribute),
								false);
}

void ObjectTemplate::SetInternalFieldCount(int value)
{
	GET_JSC_THIS()->setInternalFieldCount(value);
}

void ObjectTemplate::SetHandler(const NamedPropertyHandlerConfiguration& configuration)
{
	// Taken from v8's CreateInterceptorInfo in api.cc
	DCHECK(configuration.query == nullptr || configuration.descriptor == nullptr);  // Either intercept attributes or descriptor.
	DCHECK(configuration.query == nullptr || configuration.definer == nullptr); // Only use descriptor callback with definer callback.

	GET_JSC_THIS()->setInterceptors(configuration);
}

void ObjectTemplate::SetHandler(const IndexedPropertyHandlerConfiguration& configuration)
{
	// Taken from v8's CreateInterceptorInfo in api.cc
	DCHECK(configuration.query == nullptr || configuration.descriptor == nullptr);  // Either intercept attributes or descriptor.
	DCHECK(configuration.query == nullptr || configuration.definer == nullptr); // Only use descriptor callback with definer callback.

	GET_JSC_THIS()->setInterceptors(configuration);
}

void ObjectTemplate::SetCallAsFunctionHandler(FunctionCallback callback, Local<Value> data)
{
	GET_JSC_THIS()->setCallAsFunctionHandler(callback, data.val_);
}

} // v8