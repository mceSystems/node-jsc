/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"

#include <JavaScriptCore/JSONObject.h>
#include <JavaScriptCore/JSCInlines.h>

namespace v8
{

MaybeLocal<Value> JSON::Parse(Local<Context> context, Local<String> json_string)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context);
	JSC::JSString * json = jscshim::GetJscCellFromV8<JSC::JSString>(*json_string);
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);

	return Local<Value>(JSC::JSONParse(exec, json->tryGetValue()));
}

// Based on JSONStringify from JSC's JSONObject.h
MaybeLocal<String> JSON::Stringify(Local<Context> context, Local<Object> json_object, Local<String> gap)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context);
	DECLARE_SHIM_EXCEPTION_SCOPE(*context);

	// JSC::JSONStringify doesn't handle empty values (will crash)
	JSC::JSValue jscGap = gap.val_.isEmpty() ? JSC::jsUndefined() : gap.val_;

	WTF::String json = JSC::JSONStringify(exec, json_object.val_, jscGap);
	SHIM_RETURN_IF_EXCEPTION(Local<String>());

	return Local<String>(JSC::JSValue(JSC::jsString(exec, json)));
}

} // v8