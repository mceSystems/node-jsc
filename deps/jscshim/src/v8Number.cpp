/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include "shim/helpers.h"

#include <JavaScriptCore/PureNaN.h>
#include <JavaScriptCore/JSCInlines.h>

namespace v8
{

double Number::Value() const
{
	return jscshim::GetValue(this).asNumber();
}

Local<Number> Number::New(Isolate * isolate, double value)
{
	return Local<Number>::New(JSC::jsNumber(JSC::purifyNaN(value)));
}

} // v8