# jschim
jscshim is the core of node-jsc, implementing v8's api (shim) on top of JavaScriptCore (JSC), WebKit's javascript engine. This is similar to [node-chakracore](https://github.com/nodejs/node-chakracore)'s ChakraShim and [spidernode](https://github.com/mozilla/spidernode)'s spidershim.
Since javascript engines are different and provide different APIs, they might not provide the same functionality to embedders, and mimicking one engine's exact API on top of another engine is not always possible. But JavaScriptCore is powerful and flexible enough to allow implementing most of v8's API, so hopefully such differences shouldn't be an issue for node and for native extension developers. 
This document describes some (but definitely not all) of the key design\implementation details of jscshim, the differences between v8 and JSC and how jscshim tries to deal with them.

## JavaScriptCore API
There are two options for working with JavaScriptCore and embedding it:
- **Use JavaScriptCore's official API**: This is "official" API offered by JavaScriptCore and documented by [Apple's JavaScriptCore Framework documentation](https://developer.apple.com/documentation/javascriptcore) (and in the API headers). It provides an easy to use API for using JavaScriptCore either from Objective-C or C. 
  Internally it is implemented on top of JavaScriptCore's "internal" classes and functions.
- **Directly work with JavaScriptCore "internal" classes\functions**, most of them are exported by JavaScriptCore. It is more complicated, much less documented and requires more knowledge of how JavaScriptCore works. But this approach provides a much more powerful and flexible access to JavaScriptCore, and provides functionality that can't be achieved by using the official API.
  This approach is used by WebKit itself (in WebCore) and by [NativeScript](https://www.nativescript.org/).

For jscshim, the official API didn't cover everything needed, thus I've taken the second approach.

## WebKit Version
jscshim uses it's own fork of WebKit (which is pretty up to date), since a few modifications\additions were made to it (hopefully at least some of them will be merged upstream). See the ["WebKit fork and compilation"](https://github.com/mceSystems/node-jsc/blobl/master/deps/jscshim/docs/webkit_fork_and_compilation.md) document for more information about what was changed\added, or go to our fork's [repo](https://github.com/mceSystems/WebKit) and [see the changes](https://github.com/mceSystems/webkit/compare).

## Isolates and Contexts
v8::Isolate is in isolated instance of v8. A single isolate can have multile v8::Contexts - sandboxed, separated, execution environment. This roughly correlates to JSC::VM (also called "Context Group" in JSC's API) and JSC::JSGlobalObject ("Context" in JSC's API), even if they are not exactly the same.
In jscshim, v8::Isolate wraps a JSC::VM instance (and fills in the rest of the necessary functionality) and v8::Context is in instance of jscshim's subclass of JSC::JSGlobalObject (and is a heap allocated, garbage collected, object like v8's Context).

## Values and Garbage Collection
v8 and JSC represent javascript values and objecs differently. Although the exact internal details aren't important, if you are interested in reading more about it:
* [value representation in javascript implementations](http://wingolog.org/archives/2011/05/18/value-representation-in-javascript-implementations) - A bit older but still relevant and provides a good explanation.
* [JSC's JSValue.h](https://github.com/WebKit/WebKit/blob/master/Source/JavaScriptCore/runtime/JSCJSValue.h) - Provides a good explanation of the NaN encoding method used by JSC.
* [Attacking JavaScript Engines: A case study of JavaScriptCore and CVE-2016-4622](http://phrack.org/papers/attacking_javascript_engines.html) - Don't be intimidated by the article's name (or source), it provides an excellent explanation of core JSC topics like value representation, the object model and structures.

In general, javascript values in JSC are represented by a 64-bit JSC::JSValue, which may be a primitive value, or a pointer to a heap object (JSC::JSCell). It is important to note that since JSValues are always 64-bit, they can't be represented as (or casted to) pointers on 32-bit systems. This is different from v8's representation, where values are pointer size.

### Garbage Collection and Handles
As javascript is a garbage collected language, the garbage collector has to know which objects are used\referenced from native code. In v8, this is done using "handles", mainly v8::Local and v8::Persistent:
- v8::Local instances are supposed to be stack allocated, and register their values with the current v8::HandleScope. 
- "Persistent" handles (mainly v8::Persistent and v8::Global), which allows more control on the object's lifetime.
For a more detailed explanation, see the "Handles and Garbage Collection" section in [v8's Embedder's Guide](https://github.com/v8/v8/wiki/Embedder%27s-Guide).

In JSC, on the other hand, the GC scans the native stack looking for pointers to the javascript heap, thus there's no need for special handles. Thus, in jscshim:
- v8::Local simply holds a JSC::Value, and we count on the fact (and enforce it) that v8::Local instances are meant to be stack allocated, thus their value will be seen by the GC.
  Note that since JSValues are always 64-bit, we can't just store "T \*" like v8 does on 32-bit systems, and we can't simply cast JSC::Values to "T \*" in the "->" and "\*" operators. In these cases, we'll return a pointer to the JSValue we're holding (which is valid as long as the Local instance is valid). This might be a bit confusing internally in jscshim (for classes which represent heap objects), as the pointer we're returning is actually a pointer to the JSValue, which itself is a pointer to the actual heap object.
  See v8::Local class documentation in jscshim's v8.h for more information.
- HandleScopes are not needed, and their implementation is empty.
- To implement persistent handles (mainly v8::Persistent), we use JSC::gcProtect\gcUnprotect to make sure the object is still alive as needed, and JSC::Weak to support finalization callbacks.

See [WebKit's blog post about the "Riptide" garbage collector](https://WebKit.org/blog/7122/introducing-riptide-webkits-retreating-wavefront-concurrent-garbage-collector/) for more information about JSC's garbage collector.

## Exceptions
From a native code point of view, while providing similar basic functionality (catching and throwing exceptions, retrieving exception information, etc.) v8 and JSC do things differently:
- v8:
  - The v8::TryCatch class acts like an "external" try\catch block, and is treated like one: it can catch exceptions, clear or rethrow them, etc.
  - v8::TryCatch::Exception() returns the value that was thrown.
  - An exception might have a v8::Message object associated with it. Message objects hold the exception message and source information.
  - v8::Isolate instances can have "message listeners" (added with AddMessageListener). If an exception wasn't handled, or if the current TryCatch handler is set to "verbose" (and to capture the message objects), it's message object (if present) will be forwarded to the listeners.
- JSC:
  - To catch\throw exceptions from native code, JSC::CatchScope\JSC::ThrowScope are used. But a JSC::CatchScope isn't the same as a catch block, it just provides access to the thrown exception, which is held by the vm. The JSC::CatchScope is not an actual try\catch block and won't "hold" the exception.
  - Exceptions are represented by JSC::Exception instances, which hold the value that was thrown and possibly a stack trace.
  - There is no equivalent to v8::Message (and message listeners): the source information could be found through the stack trace. Thus, source information is only available if a stack trace was captured.
  - v8's ["Stack Trace API"](https://github.com/v8/v8/wiki/Stack-Trace-API) (Error.captureStackTrace) isn't implemented.

jschim's v8::TryCatch and v8::Message (and the corresponding parts in v8::Isolate) implementations try to mimic v8's behavior as much as possible, but some differences\issues might still be present. Currently, for example, if an exception is cauhgt and rethrown, the stack trace provided by the v8::Message instance should provide the original stack trace, while we currently provide the new ("rethrown") stack trace (for more limitations and known issues, see [jscshim's status and known issues](https://github.com/mceSystems/node-jsc/blobl/master/deps/jscshim/docs/jscshim_status_and_issues.md)).

A basic implementation of Error.captureStackTrace is provided for users by jscshim, but it's not currently used internally by JSC to format the stack trace of Error instances.

## JSC and WTF Headers
Some of the class declarations in jscshim's v8.h require using JSC (and WTF) classes and types. In jscshim, we'll include the JSC\WTF headers we need. But, including those headers in node (which uses v8.h everywhere) is problematic:
- The main problem is class name collisions with WTF ("Web Template Framework"), which adds every class to the global namespace ("using WTF::*" at the end of each file). I tried removing these "using" statements but it broke JSC headers compilation and required too many changes.
- Not as important as the previous issue, but including JSCJSValue.h and JSCJSValueInlines.h will include a few other JSC\WTF headers, increasing compilation time (and possibly size).

Since we only need a few types, jscshim solves this issue in two ways:
- For all types other then JSC::JSValue, we either just need a forward declaration, or just need to know their size (they are only actually used inside jscshim). To use these types, we'll use "place holder" types generated by the jscshim_jsc_types_generator (see documentation below), which are basically types containing an unused array with the original class' size. This might seem hacky, and it is. But, it lets us use JSC types more naturally and avoid heap allocations (pimpl) when possible. WebKit's build process already uses a few scripts with similar ideas (which actually inspired this solution) for generating the interperter code (see WebKit's "offlineasm").
So, to provide jscshim with the needed headers, but node with just the "place holders"\JSC::JSValue replacement, our v8.h contains:
  ```c
  #ifdef BUILDING_JSCSHIM
    // In jscshim - include the real JSC/WTF headers and types
    #include "jsc/config.h"
    #include <JavaScriptCore/JSCJSValue.h>
    #include <JavaScriptCore/ThrowScope.h>
    ...
  #else
    #include "jsc-types.h" // Contains the place holders and our stripped down JSC::JSValue implementation
  #endif
  ```
- For JSC::JSValue, the above solution is not enough: JSC::JSValues are used by v8::Local (and v8::Persistent) instances which are used pretty much everywhere by v8 users. Thus, we'd prefer to avoid calling into node every time we use a Local, and keep it a header only class. Since a JSC::JSValue is just a 64-bit value, and we only need a couple of it's methods in v8.h, I've created a tiny, stripped down, version of JSC::JSValue which is compatible with the "complete" type. This is hacky, but the basic implementation of JSC::JSValue is unlikely to change in the near future. See the "Values and Garbage Collection" section above for more detailed information about JSC::JSValue and v8::Local.

### jscshim_jsc_types_generator
jscshim_jsc_types_generator generates the "place holder" types described above, which allows us to use JSC\WTF types in v8.h without including JSC\WTF headers. 
For example, a "place holder" for JSC::JSObject:
```c
class JSObject { private: char m_placeHolder[24]; };
```
It is a bit hacky, but gets the job down for now.

To generate a place holder, jscshim_jsc_types_generator only needs to know the size of the type. Theoretically, this sounds easy: jscshim_jsc_types_generator could be an executable that just "printf" the sizeof(type) of each type, formatted into a class definition. When building jscshim, we could run jscshim_jsc_types_generator and generate a "place holders header" before complinng jscshim and node.
But since we'll be cross compiling for iOS, this might not work as types\classes might have different sizes (due to conditional compilation, different processsor might require different alignments, etc.). Thus, jscshim_jsc_types_generator must be (cross) compiled for the target platform, therefore we might not be able to run it on our host. To solve this, jscshim_jsc_types_generator mimics WebKit's offlineasm (the tool that generates the platform specific code for LLInt, JSC's interpreter): A binary is compiled for the target platofrm, containing the needed sizes, and a script later extracts them and generates the final header. see generate_jsc_types_header.py for more information.

## Objects and Functions
In v8's API, objects and functions are created using templates (v8:ObjectTemplate\::FunctionTemplate) or directly by v8::Object\v8::Function. Most of this functionality is implemented by jscshim, but there are a few more challenging areas:
- Interceptors: Callbacks called when an object property is accessed (either by name or by index). In v8, the callbacks are set on the ObjectTemplate. In JSC, in the C++ API we're using, things are different: Subclasses of JSC::JSCell (heap objects) can override functions in the class' "method table", which are used by JSC when accessing the object's properties.
Thus, when an object has interceptors set for it, jscshim overrides the necessary JSC class methods and forwards the calls to the user callbacks. Note that altough it sounds similar, there's no exact correlation between the types of interceptors in v8 and the methods in JSC's class method table. jscshim tries its best to translate them as close as possible.
See jscshim's ObjectWithInterceptors implementation for more information.
- Accessors: C++ callbacks which are invoked when an object's property is accessed, but appear as regular (data) properties to the "outside world". JSC didn't have the exact functionaliy needed in order to implement accessors, so I added it (See the badly named "CustomAPIValue" in our WebKit fork).
- Hidden prototyeps: JSC doesn't have the concept of hidden prototypes. Most of the hidden prototypes functionality is implemented in jscshim, by the relevant API parts (FunctionTemplate, Signature handling, etc.) and by replacing Object.prototype.__proto__'s getter and setter with our own implementation (see jscshim's shim/GlobalObject.cpp for more detailed information). But some functionality is still missing and is harder\more costly to implement. For example (from v8's unit tests): "Setting a property that exists on the hidden prototype goes there".
- Cloning: JSC doesn't seem to provide something similar to v8::Object::Clone, so I've implemented it in jscshim. It still needs some verification.
  See jscshim's v8::Object::Clone (in v8Object.cpp) for detailed information about how it was implemented.

## Function argument access in v8::FunctionCallbackInfo
From jscshim's FunctionCallbackInfo class documentation in v8.h:
- In v8, the "[]" operator simply accesses the stack (through a local internal::Object** member).
- In JSC, accessing function arguments is done though the JSC::ExecState passed to functions, which provides inline functions for accessing arguments. 
- JSC::ExecState is not a "real" object - it actually points to an array of "Registers".  Each Register has the following internal represnetation (taken from JSC's Register.h):
    ```c
    union {
      EncodedJSValue value;
      CallFrame* callFrame;
      CodeBlock* codeBlock;
      EncodedValueDescriptor encodedValue;
      double number;
      int64_t integer;
    } u;
    ```
- Function arguments are continuous in the Register array.

So, we have a few possible ways to access the arguments from FunctionCallbackInfo:
- Include the necessary headers directly an use the ExecState passed to the calling function. Refer to the previous comments on using JSC\WTF headers regarding why this is problematic.
- Provide ש helper function\class exoported from jscshim (node's binary). The downside here is having to make a call into jscshim\node for every argument access.
- Copy the arguments part of the Registers array into a new buffer, like JSC's *Arguments classes (DirectArguments, etc.) do, and pass it here.
- Simply pass the argument count and a pointer to the first argument, and access it directly from here. Since we know we're accessing JSValues, and the size of a Register is 64bit (see the union above), we can treat the Register array as a JSValue array.

The first approach would have been the best option, if we could easily include JSC's headers here (which we can't). The last option is a more hacky\dirty version of the first approach, relying on the internal structure of ExecState\Register. But, it seems that the basic idea and structure of the ExecState\Register should be stable, and changes to them will probably require us to review the changes anyway. So for now, in order to avoid the overhead of the other options, we'll go with the last one.

## Unit Tests
jscshim is tested using v8's unit tests, modified when needed. It currently passes almost 300 unit tests which cover it's core functionality. While some of the unit tests still haven't been tested\modified, some tests:
- Are irrelevant as they test v8 specific functionality or internal APIs.
- Behave differently, mainly due to differences in the garbage collection. This fail some tests (as finalizers will be called at different times, for example).
- Test APIs still not implemeneted by jscshim. 

Note that because **v8:ArrayBuffer::Externalize is currently broken** (see ["ArrayBuffer and Custom Allocators" in jscshim Status and Known Issues](https://github.com/mceSystems/node-jsc/blobl/master/deps/jscshim/docs/jscshim_status_and_issues.md)), ScopedArrayBufferContents instances (test-api.cc) won't try to free buffers and will currently leak memory.

### iOS Test Runner
The iOS test runner, jscshim-cctest (Xcode Project), can be found under deps/jscshim/test/tools/cctest-ios-runner.
Currently it needs to be compiled and run manually, which is not ideal, but a more automated solution is planned.

### Windows
To run jscshim's unit tests, it is recommeded to use the test runner (which is s slightly modified version of v8's test runner). From the command line, in node-jsc's root directory, run jscshim's run-tests.py:
```
python deps/jscshim/test/tools/run-tests.py --outdir=..\..\..\Release
```
(or "Debug" if it is a debug build).

```run-tests.py``` will use ```jscshim_tests.exe``` (found at the output directory) to actually run the tests. If needed, it is also possible to use ```jscshim_tests.exe``` directly, to either run all tests (which is supported by jscshim, but less recommeded and will result "deprecated" warnings) or a specific test:
* Run all tests:
  ```jscshim_tests.exe test-api```
* List all available tests: 
  ```jscshim_tests.exe --list```
* Run a specifc test: 
  ```jscshim_tests.exe test-api/CorrectEnteredContext```
