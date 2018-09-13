/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "v8-inspector.h"
#include "V8InspectorImpl.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8_inspector
{

std::unique_ptr<V8Inspector> V8Inspector::create(v8::Isolate * isolate, V8InspectorClient * client)
{
	// TODO: IMPLEMENT
	return std::make_unique<V8InspectorImpl>(isolate, client);
}

}