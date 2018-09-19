# jscshim Status and Known Issues
Implementing all of v8 APIs is a difficult and probably unnecessary mission, since:
- v8 API is quite extensive, and not everything is used by node.
- Differences between v8 and JSC in various areas (javascript source compiling and cache management, garbage collection, snapshot support, etc.) make some features hard or even impossible to implement, or even irrelevant in JSC.

As jscshim targets node, and is a work in progres, it currently implements essential parts of v8 APIs needed to support most parts of node, and hopefully native extensions. But, this means:
- Some functions\classes might be missing from jscshim's v8.h.
- Some functions\classes might have empty implementations, and some function might not handle\implement specific arguments.
- Some functions\classes might behave differently in jscshim.
If you've found a bug or need some missing APIs - feel free to create an issue on GitHub, or even better - a pull request.

This document doesn't cover everything that is different or missing, but tries to cover key areas and known issues. If you think this document is missing something (and it probably is, as its not easy to cover everything) - please let us know.

## Platform & "Message Pump"
Currently, as JSC doesn't support something similar to v8's v8::Platform abstraction, it is not implemented by jscshim. Thus, node-jsc is compiled without NODE_USE_V8_PLATFORM.

## Handles
- In jscshim, v8::Local instances must be stack allocated, as we rely on JSC's garbage collector to "see" the underlying JSC::JSValue when it scans the native stack. This lets us avoid the overhead of having to manually "protect" the value, which is unnecessary as Local instances are intended to be stack allocated.
- v8::Persistent: finalization callbacks (set with "SetWeak") might be called at different times in v8 and in jscshim (JSC). This is valid, as v8's docucmentation explictly says "There is no guarantee as to *when* or even *if* the callback is invoked". But, in node, I was facing crashes when callbackes where invoked during the VM's desturctor (called when the Isoalte is being disposed). After some investigation, I suspected that node's "weak callbacks" seem to rely on being called earlier, thus they access node's Environment object, Isolate, etc., which might not be legal during the VM\Isolate destruction (acessing already freed objects\memory, etc.). To help protect from this issue, jscshim won't call the user supplied callback when the VM is being destroyed. While this might not protect\fix all cases, it does seem to fix the current issue. 
  I plan on creating an issue in node's repo for this.

## Locking
Both v8 and JSC support accessing Isolates\VMs from different threads by using locks (thus only one thread can access it at a time). In v8, an Isolate can be locked with a v8::Locker, while in JSC this can achieved either by using a JSC::JSLockHolder, or by manually accessing the VM's apiLock.
In JSC, acquiring a JSLock involved some "context switching" (change the VM stack, set the thread's atomic string table), while unlocking also triggers some "house keeping" tasks like draining micro tasks. Because this, I was afraid of the performance cost of keep acquiring and releasing the lock on every call on mobile devices. Thus, as in node the Isolate is accessed only from one thread (which should also remain true in node's [experimental worker thread support](https://nodejs.org/api/worker_threads.html)), jscshim::Isolate::New acquires the VM's apiLock (and releases it upon destruction). As I haven't measured it, I know this is an evil premature optimization, and as node's ("main") thread locks it's isolate, calls whould have been done when the lock is already lock (thus having much less overhead). So this issue needs some measuring, so that jscshim could hopefully achieve proper locking support.

Note that this has created a challange for [node-native-script](https://github.com/mceSystems/node-native-script), but see it's documentation for more information.

## Script Compilation and Execution
In v8::Script, there's a distinct separation between compiling the script (done through v8::ScriptCompiler) and running it (with v8::Script::Run). This separation is less straightforward in JSC, and is currently not implemented in jscshim. This means:
- All of the work is currently done in v8::Script::Run, which actually compiles and runs the script.
- As v8::Script::Compile and and v8::ScriptCompiler::Compile don't actually compile the script, they won't fail because of invalid scripts\syntax errors.

## ArrayBuffer and Custom Allocators
JSC doesn't support custom ArrayBuffer allocators, and will use either the system allocator or it's own allocator, bmalloc. A custom allocator support in node is important for two main reasons:
- node provides a custom allocator to control whether the allocated buffers will be initialized to zero or not. This allows node to allocate uninitialized ArrayBuffers, as an optimization, used by **Buffer.allocUnsafe** and **Buffer.allocUnsafeSlow**. 
- **ArrayBuffer::Externalize**: Users are resposible for freeing the buffer returned by ArrayBuffer::Externalize, which should have been allocated by their provided allocator, or by v8's default one (which uses malloc\calloc). This presents a problem when JSC uses bmalloc.

jscshim currently doesn't support custom allocators, which means:
- **Buffers in node are allways filled with zeros**, meaning we're currently losing an optimization.
- **ArrayBuffer::Externalize is currently broken**, as it might return a buffer allocated by bmalloc.

We can solve this in two possible ways:
- **Add support for custom allocators**: this could be done in several ways:
  - Implement it directly in JSC to support a per VM custom allocator.
  - To avoid modifying JSC, jscshim could replace the global ArrayBuffer constructor with our own constructor.
- Solve node's main use of custom allocators - **support allocating uninitialized ArrayBuffers**. This will require:
   - Exposing a way (maybe through a new global function) to allocate an uniniailzied ArrayBuffer. Internally, JSC::ArrayBuffer::create already expose this functionality through ArrayBufferContents::InitializationPolicy.
   - Change node's createUnsafeArrayBuffer (in lib/buffer.js) to use our function when using jscshim. 

   Note that this won't solve ArrayBuffer::Externalize's problem. An inefficient solution for it could be to to copy the ArrayBuffer's contents into a newly allocated buffer, allocated with the custom allocator.

Note that **SharedArrayBuffer** is currently disabled by default in JSC due to WebKit's Spectre mitigations, and jscshim leaves it disabled.

## Objects and Functions
- **Hidden prototypes**: Most of the functionality is implemented, but "Setting a property that exists on the hidden prototype goes there" (from v8's unit tests) is not implemented.
- **v8::Object::GetPropertyNames**: The KeyConversionMode argument controls whether indices will be kept as numbers or will be convereted to strings, and defaults to kKeepNumbers. 
  This is currently not supported in jscshim, and indices will always be returned as strings. Supporting this will require some work, since JSC::Identifier always converts numbers to strings (and in JSC, PropertyNameArray arrays contain JSC::Identifier instances).
- **v8::Object::Clone**: As JSC doesn't offer this exact functionality (besides calling Object.assign), I've implemented it myself. It needs some more testing and verification, which I hope to do with WebKit's developers.
- **v8::Function::GetInferredName** and **v8::Function::GetDebugName**: v8 seems to add the "full" name of the functions, including member names. For example, from v8's unit tests:
  ```javascript
  var foo = { bar : { baz : function() {}}}; var f = foo.bar.baz;
  ```
  for foo, GetInferredName will return "foo.bar.baz". According to the "FunctionGetDebugName" v8 unit test, it seems that GetDebugName might also return it. This currently does not seem to be supported in JSC, thus in jscshim. 

## Exception Handling
- **v8::Message**:
  - There seem to be some differences in the exception's source information between v8 and JSC, specifically source offsets (start column, end column). jscshim's v8::Message implementation tries to calculate everything as close to v8 as possible, but some differences might still occur. See jscshim's [shim/JSCStackTrace.cpp](https://github.com/mceSystems/node-jsc/blobl/master/deps/jscshim/src/shim/JSCStackTrace.cpp) for more information.
  - jscshim's v8::Message implementation uses the exception's stack trace to get source information. If an exception doesn't have a stack trace, the current one (at the point where the Message object is created) will be used.
  - If an exception is cauhgt and rethrown, the stack trace provided by the v8::Message instance should provide the original stack trace, while we currently provide the new ("rethrown") stack trace.
- [Error.captureStackTrace](https://github.com/v8/v8/wiki/Stack-Trace-API):
  This is a non stanard api implemented by v8. Currently, jscshim offers a basic implemenation of it, but:
  - It is not currently used internally by JSC to format the stack trace of Error instances, like in v8. It is provided to support users calling it directly.
  - Error.stackTraceLimit is ignored. 
  - Some of the CallSite's functions haven't been implemented (although they are all present).

## Promises
- **Hooks** are not implemented. As this is not currently supported by JSC it could be added, but might have a performance impact on promises, thus must be carefully implemented, tested and measured.

## Microtasks
- Isolate::EnqueueMicrotask has not been implemented yet.
- The Isolate's MicrotasksPolicy is currently ignored.

## ES6 Modules
Currently not implemented.

## ValueSerializer and ValueDeserializer
Currently not implemented.