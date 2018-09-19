# WebKit Fork and Compilation
jscshim and node-jsc ship with WebKit, which is compiled during the node-jsc build process.

## WebKit Port and Compilation
jscshim\node-jsc's initial goals for building WebKit:
- Use just what we need from WebKit: JavaScriptCore and WTF ("Web Template Framework").
- Compile JSC and WTF as a static libraries, if possible.
- Use the optimized assembly version of LLInt (JSC's interpreter), not cloop. This requires enabling JIT support, although we won't be using the JIT (but we can omit the FTL jit).

WebKit provides a several building options:
- CMake: Offers cross platform support and customization options, through several different "ports": AppleWin, WinCairo, Mac, etc.
  - A port JSCOnly, (originally) not intended for production, aims to make it easier to build just JavaScriptCore, reducing dependencies as much as possible.
  - It seems that the CMake scripts are sometimes outdated\not as maintained as the other, platform specific, options (at least compated to the XCode projects).
- For macOS and iOS, there are Xcode projects.
- For Windows there are vcxproj\sln files.
Theses options can be used directly, or through WebKit's building scripts: build-webkit and build-jsc (under Tools/Scripts).

CMake seemed like a good options, since it will allow us to support multiple platform more easily, and it offers the "JSCOnly" port. JSCOnly port, compiled as a static library, is exactly what we need. The downside of using the JSCOnly port is that it wasn't intended for production use, thus have some limitations (like not leveragin CF RunLoops on iOS\macOS).
Still, **jscshim currently use CMake to build the JSCOnly port (after some modifications)**.

When building node-jsc, jscshim gyp (webkit.gyp) will invoke jscshim's build_jsc.py to build WebKit:
- For any target other than iOS, build_jsc.py will use WebKits build-jsc script.
- After having trouble with compiling JSCOnly port for iOS with build-jsc, I decided to skip build-jsc and use cmake\xcodebuild direclty, providing cmake with a custom iOS "toolchain" file.
  I used NativeScript as a reference, as they are also compiling WebKit from cmake, although they are doing a few things differently (they compile it as part of their own cmake compilation, and they use the "Mac" port).

See [build_jsc.py](https://github.com/mceSystems/node-jsc/blobl/master/deps/jscshim/tools/build_jsc.py) for more detailed information and configuration paramters used.

## WebKit Fork
During the development of jscshim, I needed to make some changes to WebKit, thus we have our own [fork](https://github.com/mceSystems/webkit). The changes were mainly either to add needed features, fix problems or support the JSCOnly compilation. Hopefully we could merge at least some of the upstream.

The major changes\addtions in our fork:
- Added [CustomAPIValue](https://github.com/mceSystems/webkit/commit/d339fdcd794dc9002a82b66e1c9bda37228777b6): Similar in functionality to v8's accessor (and used to implement them), allowing object properties that appear like data properties, but invoke native callbacks when accessed.
- Expose more headers (in ForwardingHeaders), classes, members and functions needed by jscshim and node-native-script.
- MSVC\Windows related:
  - MSVC compilation fixes.
  - [offlineasm parser should handle CRLF in asm files](https://github.com/mceSystems/webkit/commit/06f9f97064202537808264d5a1d95b565df3c97f).
- [Added support for private registered symbols](https://github.com/mceSystems/webkit/commit/d1c5146adf6c105c546e6b019f6f2342876afe22)
- [Added "external" string support to WTF::WTFString](https://github.com/mceSystems/webkit/commit/9a812c95f14c25a93c9cad600904de2aa4484941), which are strings that user allocated, but users provide a custom "free" function.
- [JSONStringify can now accept a custom gap](https://github.com/mceSystems/webkit/commit/4621f0eb3798a3aabddc0afd8a8326d4b1b2eb2e)
- ArrayBuffers:
  - [Support creating ArrayBuffers "around" user controlled buffer](https://github.com/mceSystems/webkit/commit/a5f945008c2b524c5ad405275ec502e1155a7e70), without copying or freeing them.
  - [Allow neutering of (all) ArrayBuffers](https://github.com/mceSystems/webkit/commit/eef08cbbff3af3e608fc7eacbfd8066f71b9dc10)
- JSCOnly port related:
  - [Allow "custom icu" headers and libs to be easily configured](https://github.com/mceSystems/webkit/commit/9279b1fdb86ad861608ae877cf1259d9863e82aa).
  - on iOS\macOS, [enable "USE_FOUNDATION"](https://github.com/mceSystems/webkit/commit/3d5200a94d09d81420ba5b499c28a381792ef081) and [WTF::RetainPtr](), needed for [node-native-script](https://github.com/mceSystems/node-native-script) (enables JSC::Heap::releaseSoon).