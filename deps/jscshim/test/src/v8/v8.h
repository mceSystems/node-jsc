// Copyright 2011 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_V8_H_
#define V8_V8_H_

#include "include/v8.h"
#include "allocation.h"
#include "globals.h"

namespace v8 {
namespace internal {

// jscshim: TODO
class V8 : public AllStatic {
 public:
  // Global actions.

  //static bool Initialize();
  //static void TearDown();

  // Report process out of memory. Implementation found in api.cc.
  // This function will not return, but will terminate the execution.
  static void FatalProcessOutOfMemory(const char* location,
                                      bool is_heap_oom = false)
  {
	  printf("FatalProcessOutOfMemory at %s", location);

	  // Taken from WTF's Crash
	  *(int *)(uintptr_t)0xbbadbeef = 0;
	  ((void(*)())0)();
  }

  //static void InitializePlatform(v8::Platform* platform);
  //static void ShutdownPlatform();
	 V8_EXPORT_PRIVATE static v8::Platform* GetCurrentPlatform() { return nullptr; }
  // Replaces the current platform with the given platform.
  // Should be used only for testing.
 // static void SetPlatformForTesting(v8::Platform* platform);

 // static void SetNativesBlob(StartupData* natives_blob);
 // static void SetSnapshotBlob(StartupData* snapshot_blob);

 //private:
 // static void InitializeOncePerProcessImpl();
 // static void InitializeOncePerProcess();

 // // v8::Platform to use.
 // static v8::Platform* platform_;
};

}  // namespace internal
}  // namespace v8

#endif  // V8_V8_H_
