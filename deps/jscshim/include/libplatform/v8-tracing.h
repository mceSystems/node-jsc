// Copyright 2016 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_LIBPLATFORM_V8_TRACING_H_
#define V8_LIBPLATFORM_V8_TRACING_H_

#include <fstream>
#include <unordered_set>

#include "libplatform-export.h"
#include "v8-platform.h"  // NOLINT(build/include)

namespace v8 {

namespace platform {
namespace tracing {

class V8_PLATFORM_EXPORT TraceObject
{
};

class V8_PLATFORM_EXPORT TraceWriter {
 public:
  TraceWriter() {}
  virtual ~TraceWriter() {}
  virtual void AppendTraceEvent(TraceObject* trace_event) = 0;
  virtual void Flush() = 0;

  static TraceWriter* CreateJSONTraceWriter(std::ostream& stream);
};

class V8_PLATFORM_EXPORT TraceBufferChunk
{
public:
  explicit TraceBufferChunk(uint32_t seq);

  void Reset(uint32_t new_seq);
  
  bool IsFull() const
  {
    // TODO: IMPLEMENT
    return false;
  }
  TraceObject* AddTraceEvent(size_t* event_index)
  {
    // TODO: IMPLEMENT
    return nullptr;
  }

  TraceObject* GetEventAt(size_t index)
  {
    // TODO: IMPLEMENT
    return nullptr;
  }

  uint32_t seq() const
  {
    // TODO: IMPLEMENT
    return 0;
  }

  size_t size() const
  {
    // TODO: IMPLEMENT
    return 0;
  }

  static const size_t kChunkSize = 64;

private:
  // Disallow copy and assign
  TraceBufferChunk(const TraceBufferChunk&) = delete;
  void operator=(const TraceBufferChunk&) = delete;
};

class V8_PLATFORM_EXPORT TraceBuffer {
 public:
  TraceBuffer() {}
  virtual ~TraceBuffer() {}

  virtual TraceObject* AddTraceEvent(uint64_t* handle) = 0;
  virtual TraceObject* GetEventByHandle(uint64_t handle) = 0;
  virtual bool Flush() = 0;

 private:
  // Disallow copy and assign
  TraceBuffer(const TraceBuffer&) = delete;
  void operator=(const TraceBuffer&) = delete;
};

class V8_PLATFORM_EXPORT TraceConfig {
 public:
  void AddIncludedCategory(const char* included_category);
};

#if defined(_MSC_VER)
#define V8_PLATFORM_NON_EXPORTED_BASE(code) \
  __pragma(warning(suppress : 4275)) code
#else
#define V8_PLATFORM_NON_EXPORTED_BASE(code) code
#endif  // defined(_MSC_VER)

class V8_PLATFORM_EXPORT TracingController
    : public V8_PLATFORM_NON_EXPORTED_BASE(v8::TracingController) {
 public:
  TracingController();
  ~TracingController() override;
  void Initialize(TraceBuffer* trace_buffer);

  void StartTracing(TraceConfig* trace_config);
  void StopTracing();

protected:
	virtual int64_t CurrentTimestampMicroseconds();
};

}  // namespace tracing
}  // namespace platform
}  // namespace v8

#endif  // V8_LIBPLATFORM_V8_TRACING_H_
