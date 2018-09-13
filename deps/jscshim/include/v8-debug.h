// Copyright 2008 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_V8_DEBUG_H_
#define V8_V8_DEBUG_H_

#include "v8.h"  // NOLINT(build/include)

namespace v8 {

class V8_EXPORT Debug
{
public:
	V8_DEPRECATED("Use v8-inspector",
				  static Local<Context> GetDebugContext(Isolate* isolate));

	class EventDetails
	{
	};

	typedef void(*EventCallback)(const EventDetails& event_details);

	V8_DEPRECATED("No longer supported", static bool SetDebugEventListener(
                                           Isolate* isolate, EventCallback that,
                                           Local<Value> data = Local<Value>()));

	V8_DEPRECATED("No longer supported",
                  static void DebugBreak(Isolate* isolate));
};

}

#endif  // V8_V8_DEBUG_H_