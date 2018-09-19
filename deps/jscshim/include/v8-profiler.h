// Copyright 2010 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_V8_PROFILER_H_
#define V8_V8_PROFILER_H_

#include "v8.h"  // NOLINT(build/include)

namespace v8 {

// jscshim: TODO
//struct CpuProfileDeoptFrame {
//  int script_id;
//  size_t position;
//};
//
//}  // namespace v8
//
//#ifdef V8_OS_WIN
//template class V8_EXPORT std::vector<v8::CpuProfileDeoptFrame>;
//#endif
//
//namespace v8 {
//
//struct V8_EXPORT CpuProfileDeoptInfo {
//  const char* deopt_reason;
//  std::vector<CpuProfileDeoptFrame> stack;
//};
//
//}  // namespace v8
//
//#ifdef V8_OS_WIN
//template class V8_EXPORT std::vector<v8::CpuProfileDeoptInfo>;
//#endif
//
//namespace v8 {
//
//class V8_EXPORT CpuProfileNode {
// public:
//  struct LineTick {
//    int line;
//    unsigned int hit_count;
//  };
//
//  Local<String> GetFunctionName() const;
//
//  const char* GetFunctionNameStr() const;
//
//  int GetScriptId() const;
//
//  Local<String> GetScriptResourceName() const;
//
//  const char* GetScriptResourceNameStr() const;
//
//  int GetLineNumber() const;
//
//  int GetColumnNumber() const;
//
//  unsigned int GetHitLineCount() const;
//
//  bool GetLineTicks(LineTick* entries, unsigned int length) const;
//
//  const char* GetBailoutReason() const;
//
//  unsigned GetHitCount() const;
//
//  V8_DEPRECATE_SOON(
//      "Use GetScriptId, GetLineNumber, and GetColumnNumber instead.",
//      unsigned GetCallUid() const);
//
//  unsigned GetNodeId() const;
//
//  int GetChildrenCount() const;
//
//  const CpuProfileNode* GetChild(int index) const;
//
//  const std::vector<CpuProfileDeoptInfo>& GetDeoptInfos() const;
//
//  static const int kNoLineNumberInfo = Message::kNoLineNumberInfo;
//  static const int kNoColumnNumberInfo = Message::kNoColumnInfo;
//};
//
//class V8_EXPORT CpuProfile {
// public:
//  Local<String> GetTitle() const;
//
//  const CpuProfileNode* GetTopDownRoot() const;
//
//  int GetSamplesCount() const;
//
//  const CpuProfileNode* GetSample(int index) const;
//
//  int64_t GetSampleTimestamp(int index) const;
//
//  int64_t GetStartTime() const;
//
//  int64_t GetEndTime() const;
//
//  void Delete();
//};

class V8_EXPORT CpuProfiler {
 public:
  /*static CpuProfiler* New(Isolate* isolate);

  void Dispose();

  void SetSamplingInterval(int us);

  void StartProfiling(Local<String> title, bool record_samples = false);

  CpuProfile* StopProfiling(Local<String> title);

  void CollectSample();*/

  void SetIdle(bool is_idle);

 private:
  /*CpuProfiler();
  ~CpuProfiler();
  CpuProfiler(const CpuProfiler&);
  CpuProfiler& operator=(const CpuProfiler&);*/
};

class V8_EXPORT HeapProfiler {
 public:
  typedef RetainedObjectInfo* (*WrapperInfoCallback)(uint16_t class_id,
                                                     Local<Value> wrapper);

  void SetWrapperClassInfoProvider(uint16_t class_id, WrapperInfoCallback callback)
  {
    // TODO: IMPLEMENT
  }

  void StartTrackingHeapObjects(bool track_allocations = false)
  {
    // TODO: IMPLEMENT
  }
};

class V8_EXPORT RetainedObjectInfo {  // NOLINT
 public:
  virtual void Dispose() = 0;

  virtual bool IsEquivalent(RetainedObjectInfo* other) = 0;

  virtual intptr_t GetHash() = 0;

  virtual const char* GetLabel() = 0;

  virtual const char* GetGroupLabel() { return GetLabel(); }

  virtual intptr_t GetElementCount() { return -1; }

  virtual intptr_t GetSizeInBytes() { return -1; }

 protected:
  RetainedObjectInfo() {}
  virtual ~RetainedObjectInfo() {}

 private:
  RetainedObjectInfo(const RetainedObjectInfo&);
  RetainedObjectInfo& operator=(const RetainedObjectInfo&);
};

}  // namespace v8


#endif  // V8_V8_PROFILER_H_
