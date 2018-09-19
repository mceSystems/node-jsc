/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"
#include "shim/Script.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8
{

Local<Value> ScriptOrModule::GetResourceName()
{
	jscshim::Script * thisScript = jscshim::GetJscCellFromV8<jscshim::Script>(this);
	if (nullptr == thisScript)
	{
		return Local<Value>(JSC::jsUndefined());
	}

	return Local<Value>(thisScript->resourceName());
}

Local<PrimitiveArray> ScriptOrModule::GetHostDefinedOptions()
{
	// TODO: IMPLEMENT
	return Local<PrimitiveArray>();
}

} // v8