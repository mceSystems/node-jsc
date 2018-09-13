/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include <JavaScriptCore/ProxyObject.h>
#include <JavaScriptCore/JSCInlines.h>

#include "shim/helpers.h"

#define GET_JSC_THIS() v8::jscshim::GetJscCellFromV8<JSC::ProxyObject>(this)

namespace v8
{

Local<Object> Proxy::GetTarget()
{
	JSC::JSValue target(GET_JSC_THIS()->target());
	return Local<Object>(target);
}

Local<Value> Proxy::GetHandler()
{
	return Local<Value>(GET_JSC_THIS()->handler());
}

bool Proxy::IsRevoked()
{
	return GET_JSC_THIS()->isRevoked();
}

void Proxy::Revoke()
{
	GET_JSC_THIS()->revoke(jscshim::GetCurrentVM());
}

MaybeLocal<Proxy> Proxy::New(Local<Context> context,
							 Local<Object> local_target,
							 Local<Object> local_handler)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context);

	JSC::JSValue proxy(JSC::ProxyObject::create(exec, 
												exec->lexicalGlobalObject(),
												local_target.val_, 
												local_handler.val_));
	return Local<Proxy>(proxy);
}


} // v8