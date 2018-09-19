// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_V8_PLATFORM_H_
#define V8_V8_PLATFORM_H_

#include <memory>
#include <string>

#include "v8config.h"

namespace v8 {

class Isolate;

class Task {
 public:
  virtual ~Task() = default;

  virtual void Run() = 0;
};

class ConvertableToTraceFormat {
public:
	virtual ~ConvertableToTraceFormat() = default;

	virtual void AppendAsTraceFormat(std::string* out) const = 0;
};

class TracingController {
 public:
  virtual ~TracingController() = default;

  virtual const uint8_t* GetCategoryGroupEnabled(const char* name) {
    static uint8_t no = 0;
    return &no;
  }

  virtual uint64_t AddTraceEvent(
      char phase, const uint8_t* category_enabled_flag, const char* name,
      const char* scope, uint64_t id, uint64_t bind_id, int32_t num_args,
      const char** arg_names, const uint8_t* arg_types,
      const uint64_t* arg_values,
      std::unique_ptr<ConvertableToTraceFormat>* arg_convertables,
      unsigned int flags) {
    return 0;
  }
  virtual uint64_t AddTraceEventWithTimestamp(
      char phase, const uint8_t* category_enabled_flag, const char* name,
      const char* scope, uint64_t id, uint64_t bind_id, int32_t num_args,
      const char** arg_names, const uint8_t* arg_types,
      const uint64_t* arg_values,
      std::unique_ptr<ConvertableToTraceFormat>* arg_convertables,
      unsigned int flags, int64_t timestamp) {
    return 0;
  }

  virtual void UpdateTraceEventDuration(const uint8_t* category_enabled_flag,
                                        const char* name, uint64_t handle) {}

  class TraceStateObserver {
   public:
    virtual ~TraceStateObserver() = default;
    virtual void OnTraceEnabled() = 0;
    virtual void OnTraceDisabled() = 0;
  };

  virtual void AddTraceStateObserver(TraceStateObserver*) {}

  virtual void RemoveTraceStateObserver(TraceStateObserver*) {}
};

class Platform {
 public:
  enum ExpectedRuntime {
    kShortRunningTask,
    kLongRunningTask
  };

  virtual ~Platform() = default;

  virtual size_t NumberOfAvailableBackgroundThreads() { return 0; }

  virtual void CallOnBackgroundThread(Task* task, ExpectedRuntime expected_runtime) = 0;

  virtual void CallOnForegroundThread(Isolate* isolate, Task* task) = 0;

  virtual void CallDelayedOnForegroundThread(Isolate* isolate, Task* task, double delay_in_seconds) = 0;

  virtual bool IdleTasksEnabled(Isolate* isolate) {
	  return false;
  }

  virtual double MonotonicallyIncreasingTime() = 0;

  virtual TracingController* GetTracingController() = 0;

  virtual uint64_t AddTraceEvent(
	  char phase, const uint8_t* category_enabled_flag, const char* name,
	  const char* scope, uint64_t id, uint64_t bind_id, int32_t num_args,
	  const char** arg_names, const uint8_t* arg_types,
	  const uint64_t* arg_values, unsigned int flags) {
	  return 0;
  }

  virtual uint64_t AddTraceEvent(
	  char phase, const uint8_t* category_enabled_flag, const char* name,
	  const char* scope, uint64_t id, uint64_t bind_id, int32_t num_args,
	  const char** arg_names, const uint8_t* arg_types,
	  const uint64_t* arg_values,
	  std::unique_ptr<ConvertableToTraceFormat>* arg_convertables,
	  unsigned int flags) {
	  return AddTraceEvent(phase, category_enabled_flag, name, scope, id, bind_id,
		  num_args, arg_names, arg_types, arg_values, flags);
  }

  virtual void UpdateTraceEventDuration(const uint8_t* category_enabled_flag,
	  const char* name, uint64_t handle) {}
};

}  // namespace v8

#endif  // V8_V8_PLATFORM_H_
