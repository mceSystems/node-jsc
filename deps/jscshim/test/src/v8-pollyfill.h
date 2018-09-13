/**
 * This source code is licensed under the terms found in the LICENSE file in 
 * node-jsc's root directory.
 */

#pragma once

#define INLINE(f) inline f

#ifdef _WIN32
#include "v8/base/win32-headers.h"
#endif
#include "include/v8.h"

#include "v8/utils.h"
#include "v8/allocation.h"
#include "v8/globals.h"
#include "v8/vector.h"
#include "v8/base/atomicops.h"
#include "v8/base/platform/semaphore.h"
#include "v8/base/utils/random-number-generator.h"

#include "jscutshim/internal.h"

namespace v8
{

class Utils
{
public:
#define OPEN_HANDLE_LIST(V) \
	V(Data, Object)			\
	V(Object, JSReceiver)

#define MAKE_OPEN_HANDLE(From, To) \
	static internal::Handle<internal::To> OpenHandle(v8::From * that, bool allow_empty_handle = false) \
	{ \
		DCHECK(allow_empty_handle || that != NULL); \
		return internal::Handle<internal::To>(reinterpret_cast<internal::To *>(that)); \
	}

	OPEN_HANDLE_LIST(MAKE_OPEN_HANDLE)

	#undef MAKE_OPEN_HANDLE
	#undef OPEN_HANDLE_LIST

	template <class From, class To>
	static inline v8::internal::Handle<To> OpenHandle(v8::Local<From> handle) {
		return OpenHandle(*handle);
	}

	static inline Local<Object> ToLocal(const internal::Handle<internal::JSObject>& obj)
	{
		return Local<Object>(obj.m_value);
	}
};

namespace base {

class V8_BASE_EXPORT Thread {
 public:
  // Opaque data type for thread-local storage keys.
  typedef int32_t LocalStorageKey;

  class Options {
   public:
    Options() : name_("v8:<unknown>"), stack_size_(0) {}
    explicit Options(const char* name, int stack_size = 0)
        : name_(name), stack_size_(stack_size) {}

    const char* name() const { return name_; }
    int stack_size() const { return stack_size_; }

   private:
    const char* name_;
    int stack_size_;
  };

  // Create new thread.
  explicit Thread(const Options& options);
  virtual ~Thread();

  // Start new thread by calling the Run() method on the new thread.
  void Start();

  // Start new thread and wait until Run() method is called on the new thread.
  void StartSynchronously() {
    start_semaphore_ = new Semaphore(0);
    Start();
    start_semaphore_->Wait();
    delete start_semaphore_;
    start_semaphore_ = NULL;
  }

  // Wait until thread terminates.
  void Join();

  inline const char* name() const {
    return name_;
  }

  // Abstract method for run handler.
  virtual void Run() = 0;

  // Thread-local storage.
  static LocalStorageKey CreateThreadLocalKey();
  static void DeleteThreadLocalKey(LocalStorageKey key);
  static void* GetThreadLocal(LocalStorageKey key);
  static int GetThreadLocalInt(LocalStorageKey key) {
    return static_cast<int>(reinterpret_cast<intptr_t>(GetThreadLocal(key)));
  }
  static void SetThreadLocal(LocalStorageKey key, void* value);
  static void SetThreadLocalInt(LocalStorageKey key, int value) {
    SetThreadLocal(key, reinterpret_cast<void*>(static_cast<intptr_t>(value)));
  }
  static bool HasThreadLocal(LocalStorageKey key) {
    return GetThreadLocal(key) != NULL;
  }

#ifdef V8_FAST_TLS_SUPPORTED
  static inline void* GetExistingThreadLocal(LocalStorageKey key) {
    void* result = reinterpret_cast<void*>(
        InternalGetExistingThreadLocal(static_cast<intptr_t>(key)));
    DCHECK(result == GetThreadLocal(key));
    return result;
  }
#else
  static inline void* GetExistingThreadLocal(LocalStorageKey key) {
    return GetThreadLocal(key);
  }
#endif

  // The thread name length is limited to 16 based on Linux's implementation of
  // prctl().
  static const int kMaxThreadNameLength = 16;

  class PlatformData;
  PlatformData* data() { return data_; }

  void NotifyStartedAndRun() {
    if (start_semaphore_) start_semaphore_->Signal();
    Run();
  }

 private:
  void set_name(const char* name);

  PlatformData* data_;

  char name_[kMaxThreadNameLength];
  int stack_size_;
  Semaphore* start_semaphore_;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

}

} // v8