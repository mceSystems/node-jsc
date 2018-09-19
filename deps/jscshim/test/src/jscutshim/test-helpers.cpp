/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#define BUILDING_JSCSHIM
#include "v8-pollyfill.h"

#include "../../src/shim/helpers.h"
#include "../../src/shim/FunctionTemplate.h"
#include "../../src/shim/Object.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8 { namespace jscshim { namespace test {

bool CheckTemplateProviderPrototypes(Local<Context>				 context,
									 Local<v8::FunctionTemplate> templateProivder, 
									 Local<Value>				 templateInstancePrototype,
									 Local<Value>				 templateProivderInstancePrototype)
{
	JSC::ExecState * exec = jscshim::GetExecStateForV8Context(*context);
	jscshim::FunctionTemplate * jscTemplateProivder = jscshim::GetJscCellFromV8<jscshim::FunctionTemplate>(*templateProivder);
	jscshim::Object * jscTemplateInstancePrototype = jscshim::GetJscCellFromV8<jscshim::Object>(*templateInstancePrototype);
	jscshim::Object * jscTemplateProivderInstancePrototype = jscshim::GetJscCellFromV8<jscshim::Object>(*templateProivderInstancePrototype);

	jscshim::ObjectTemplate * prototypeTemplate = jscTemplateProivder->getPrototypeTemplate(exec);

	return ((prototypeTemplate == jscTemplateInstancePrototype->objectTemplate()) &&
			(prototypeTemplate == jscTemplateProivderInstancePrototype->objectTemplate()));
}

}}} // v8::jscshim::test