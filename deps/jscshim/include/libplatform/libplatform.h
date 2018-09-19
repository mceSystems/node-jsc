// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_LIBPLATFORM_LIBPLATFORM_H_
#define V8_LIBPLATFORM_LIBPLATFORM_H_

#include "libplatform-export.h"
#include "v8-tracing.h"
#include "../v8-platform.h"

namespace v8 {
namespace platform {

enum class IdleTaskSupport { kDisabled, kEnabled };
enum class InProcessStackDumping { kDisabled, kEnabled };

enum class MessageLoopBehavior : bool {
  kDoNotWait = false,
  kWaitForWork = true
};

V8_PLATFORM_EXPORT v8::Platform* CreateDefaultPlatform(
    int thread_pool_size = 0,
    IdleTaskSupport idle_task_support = IdleTaskSupport::kDisabled,
    InProcessStackDumping in_process_stack_dumping =
        InProcessStackDumping::kEnabled,
    v8::TracingController* tracing_controller = nullptr);

V8_PLATFORM_EXPORT bool PumpMessageLoop(
    v8::Platform* platform, v8::Isolate* isolate,
    MessageLoopBehavior behavior = MessageLoopBehavior::kDoNotWait);

V8_PLATFORM_EXPORT void SetTracingController(
    v8::Platform* platform,
    v8::platform::tracing::TracingController* tracing_controller);

}  // namespace platform
}  // namespace v8

#endif  // V8_LIBPLATFORM_LIBPLATFORM_H_
