// Copyright 2010 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_V8_PROFILER_H_
#define V8_V8_PROFILER_H_

#include "v8.h"  // NOLINT(build/include)

namespace v8 {

class HeapGraphNode;
struct HeapStatsUpdate;

typedef uint32_t SnapshotObjectId;

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

class V8_EXPORT HeapGraphEdge {
 public:
	 enum Type {
		 kContextVariable = 0,
		 kElement = 1,
		 kProperty = 2,
		 kInternal = 3,
		 kHidden = 4,
		 kShortcut = 5,
		 kWeak = 6
	 };

	 Type GetType() const
	 {
		 // TODO: IMPLEMENT
		 return kInternal;
	 }

	 Local<Value> GetName() const
	 {
		 // TODO: IMPLEMENT
		 return Local<Value>();
	 }

	 const HeapGraphNode* GetFromNode() const
	 {
		 // TODO: IMPLEMENT
		 return nullptr;
	 }

	 const HeapGraphNode* GetToNode() const
	 {
		 // TODO: IMPLEMENT
		 return nullptr;
	 }
};

class V8_EXPORT HeapGraphNode {
public:
	enum Type {
		kHidden = 0,
		kArray = 1,
		kString = 2,
		kObject = 3,
		kCode = 4,
		kClosure = 5,
		kRegExp = 6,
		kHeapNumber = 7,
		kNative = 8,
		kSynthetic = 9,

		kConsString = 10,
		kSlicedString = 11,
		kSymbol = 12,
		kBigInt = 13
	};

	Type GetType() const
	{
		// TODO: IMPLEMENT
		return kHidden;
	}

	Local<String> GetName() const
	{
		// TODO: IMPLEMENT
		return Local<String>();
	}

	SnapshotObjectId GetId() const
	{
		return 0;
	}

	size_t GetShallowSize() const
	{
		// TODO: IMPLEMENT
		return 0;
	}

	int GetChildrenCount() const
	{
		// TODO: IMPLEMENT
		return 0;
	}

	const HeapGraphEdge* GetChild(int index) const
	{
		// TODO: IMPLEMENT
		return nullptr;
	}
};

class V8_EXPORT OutputStream {
 public:
  enum WriteResult {
    kContinue = 0,
    kAbort = 1
  };
  virtual ~OutputStream() {}

  virtual void EndOfStream() = 0;

  virtual int GetChunkSize() { return 1024; }

  virtual WriteResult WriteAsciiChunk(char* data, int size) = 0;

  virtual WriteResult WriteHeapStatsChunk(HeapStatsUpdate* data, int count) {
    return kAbort;
  }
};

class V8_EXPORT HeapSnapshot {
public:
	enum SerializationFormat {
		kJSON = 0
	};

	const HeapGraphNode* GetRoot() const
	{
		// TODO: IMPLEMENT
		return nullptr;
	}

	const HeapGraphNode* GetNodeById(SnapshotObjectId id) const
	{
		// TODO: IMPLEMENT
		return nullptr;
	}

	int GetNodesCount() const
	{
		// TODO: IMPLEMENT
		return 0;
	}

	const HeapGraphNode* GetNode(int index) const
	{
		// TODO: IMPLEMENT
		return nullptr;
	}

	SnapshotObjectId GetMaxSnapshotJSObjectId() const
	{
		// TODO: IMPLEMENT
		return 0;
	}

	void Delete()
	{
		// TODO: IMPLEMENT
	}

	void Serialize(OutputStream* stream, SerializationFormat format = kJSON) const
	{
		// TODO: IMPLEMENT
	}
};

class V8_EXPORT EmbedderGraph {
public:
	class Node {
	public:
		Node() = default;
		virtual ~Node() = default;
		virtual const char* Name() = 0;
		virtual size_t SizeInBytes() = 0;
		virtual Node* WrapperNode() { return nullptr; }
		virtual bool IsRootNode() { return false; }
		virtual bool IsEmbedderNode() { return true; }
		virtual const char* NamePrefix() { return nullptr; }

	private:
		Node(const Node&) = delete;
		Node& operator=(const Node&) = delete;
	};

	virtual Node* V8Node(const v8::Local<v8::Value>& value) = 0;

	virtual Node* AddNode(std::unique_ptr<Node> node) = 0;

	virtual void AddEdge(Node* from, Node* to) = 0;

	virtual ~EmbedderGraph() = default;
};

class V8_EXPORT ActivityControl {
public:
	enum ControlOption {
		kContinue = 0,
		kAbort = 1
	};
	virtual ~ActivityControl() {}

	virtual ControlOption ReportProgressValue(int done, int total) = 0;
};

class V8_EXPORT HeapProfiler {
public:
	typedef RetainedObjectInfo* (*WrapperInfoCallback)(uint16_t class_id, Local<Value> wrapper);

	typedef void (*BuildEmbedderGraphCallback)(v8::Isolate* isolate,
											   v8::EmbedderGraph* graph,
											   void* data);

	class ObjectNameResolver {
	public:
		virtual const char* GetName(Local<Object> object) = 0;

	protected:
		virtual ~ObjectNameResolver() {}
	};

	const HeapSnapshot* TakeHeapSnapshot(ActivityControl* control = NULL,
										 ObjectNameResolver* global_object_name_resolver = NULL)
	{
		// TODO: IMPLEMENT
		return nullptr;
	}

	void SetWrapperClassInfoProvider(uint16_t class_id, WrapperInfoCallback callback)
	{
		// TODO: IMPLEMENT
	}

	void StartTrackingHeapObjects(bool track_allocations = false)
	{
		// TODO: IMPLEMENT
	}

	void AddBuildEmbedderGraphCallback(BuildEmbedderGraphCallback callback, void* data)
	{
		// TODO: IMPLEMENT
	}

	void RemoveBuildEmbedderGraphCallback(BuildEmbedderGraphCallback callback, void* data)
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

struct HeapStatsUpdate {
  HeapStatsUpdate(uint32_t index, uint32_t count, uint32_t size)
    : index(index), count(count), size(size) { }
  uint32_t index;
  uint32_t count;
  uint32_t size;
};

}  // namespace v8


#endif  // V8_V8_PROFILER_H_
