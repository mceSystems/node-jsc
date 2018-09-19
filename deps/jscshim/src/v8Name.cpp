/**
 * This source code is licensed under the terms found in the LICENSE file in
 * node-jsc's root directory.
 */

#include "config.h"
#include "v8.h"

#include <JavaScriptCore/JSCInlines.h>

namespace v8
{

int Name::GetIdentityHash()
{
	// TODO: IMPLEMENT?

	// From v8's documetation: "The return value will never be 0. Also, it is not guaranteed to be unique".
	return 1;
}

} // v8


