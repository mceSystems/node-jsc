/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8
{

Local<AccessorSignature> AccessorSignature::New(Isolate* isolate, Local<FunctionTemplate> receiver)
{
	return Local<AccessorSignature>(receiver.val_);
}

} // v8