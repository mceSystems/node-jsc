/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#include "config.h"
#include "External.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8 { namespace jscshim
{

const JSC::ClassInfo External::s_info = { "JSCShimExternal", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(External) };

}} // v8::jscshim