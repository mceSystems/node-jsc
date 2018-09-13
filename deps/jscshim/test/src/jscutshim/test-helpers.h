/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "v8.h"

namespace v8 { namespace jscshim { namespace test {

/* A helper for the "SetPrototypeTemplate" test, */
bool CheckTemplateProviderPrototypes(Local<Context>				 context,
									 Local<v8::FunctionTemplate> templateProivder, 
									 Local<Value>				 templateInstancePrototype,
									 Local<Value>				 templateProivderInstancePrototype);

bool Utf8ValueStartsWith(String::Utf8Value& value, const char * prefix)
{
	return 0 == strncmp(*value, prefix, strlen(prefix));
}

}}} // v8::jscshim::test
