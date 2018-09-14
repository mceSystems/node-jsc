# node-jsc - A node.js port to the JavaScriptCore engine and iOS
node-jsc enables node.js to use JavaScriptCore, [WebKit](https://webkit.org)'s javascript engine, allowing node.js to run on iOS devices (and other platforms supported by node.js and JavaScriptCore).
While node-jsc was successully tested on iOS (has already managed to succesfully run a test express.js project with websockets), it is a very early work in progress, and is far from being ready for production use.

Currently based on node v8.11.1, with slightly newer npm version, but will be updated to a more recent version of node.

A separate project, [node-native-script](https://github.com/mceSystems/node-native-script) native extension, allows javascript code running in node-jsc to directly call iOS platform APIs.

# How?
The core part of what makes node-jsc work is jscshim, which implements the parts of v8's API used by node.js on top of JavaScriptCore (JSC), WebKit's javascript engine.
Similar to [node-chakracore](https://github.com/nodejs/node-chakracore)'s ChakraShim and [spidernode](https://github.com/mozilla/spidernode)'s spidershim, jscshim aims to implement as much of v8's APIs as possible, as close as possible, hopefully making it transperant to node.js and native extensions.
See [jscshim's documentation](https://github.com/mceSystems/node-jsc/blob/master/deps/jscshim/docs/jscshim.md) for more detailed design and implementation information. Note that as v8 API is quite extensive, jscshim currently doesn't implement all of it. See [jscshim status and known issues](https://github.com/mceSystems/node-jsc/blob/master/deps/jscshim/docs/jscshim_status_and_known_issues.md) to see what jscshim currently implements.

jscshim uses its own fork of WebKit. See [WebKit fork and compilation](https://github.com/mceSystems/node-jsc/blob/master/deps/jscshim/docs/webkit_fork_and_compilation.md) for more information, or see our fork's [repo](https://github.com/mceSystems/WebKit).

Besides jscshim, changes were made to node's build files, source code and dependencies in order to support using jscshim (and WebKit) and to support iOS (some of it taken from the great work done in [nodejs-mobile](https://github.com/janeasystems/nodejs-mobile)).

## Why node.js on iOS?
node-jsc is experimental, but:
* Why not?
* Share code with existing node.js projects\apps.
* Intergrating a "javascript to native" bridge allows calling native APIs from javascript (this is done by [node-native-script](https://github.com/mceSystems/node-native-script), as a separate module) allowing developers to mix standard node.js code with platform specific iOS code, without the need for custom native code.
* Use your iOS device for node development and testing.
* Working on node-jsc is a great way to learn about v8, JavaScriptCore and javascript engines\concepts in general.
* In the future: Run node.js on you Apple TV? (will require bitcode)

## Why JavaScriptCore?
node-jsc's initial goal was to eventually provide a stable, production ready, node.js running on iOS devices. When I started working on it, there were a few other projects to consider:
- [node-chakracore](https://github.com/nodejs/node-chakracore): As [nodejs-mobile](https://github.com/janeasystems/nodejs-mobile) wasn't out yet (or at least I wasn't aware of it), I considered using node-chakracore and basically do what nodejs-mobile has done. But, as ChakraCore doesn't offically support iOS, I was concered about using it, as it raised a lot of questions, assuming it was possible to build it for iOS and in "interpreter mode" only (since it's not possible to allocate executable memory in iOS apps):
  - How much work will it take to make it work on iOS, arm based, devices?
  - How well will it work and perform on iOS devices?
  - What about bug fixes?
  This is not to say node-chakracore and nodejs-mobile (and ChakraCore) haven't done great job and progress, it's just that they had more "risks" regarding iOS.
- [spidernode](https://github.com/mozilla/spidernode), which seemed less active and less maintained. And again, I wasn't sure how well SpiderMonkey will support iOS.

Evantually, I chose JavaScriptCore for mainly two reasons:
- As the javascript engine used by Safari, it officially supports, and optimized for, iOS devices. 
- Officially supports "interpreter mode" only (since it's not possible to allocate executable memory in iOS apps), which is successfully used by [React Native](https://facebook.github.io/react-native/docs/javascript-environment) and [NativeScript](https://docs.nativescript.org/angular/core-concepts/ios-runtime/Overview). This was important, as it means this mode is already tested by applications built on those frameworks, hopefully providing decent (enough) performance. 
  - This will also allow integration with such projects, which bridge javascript and native code, to allow javascript code running in node-jsc access the native platform's APIs. [node-native-script](https://github.com/mceSystems/node-native-script) achieves it, based on NativeScript.

Besides that:
- JavaScriptCore has a very flexible and powerful API for embedders (when interfacing with JavaScriptCore classes directly, not through the "official API" which is too limited for our needs).
- There were already [node-chakra](https://github.com/nodejs/node-chakracore) and [spidernode](https://github.com/mozilla/spidernode), so why not node-jsc? :)

## Building
### Building for iOS on macOS
**[Homebrew](https://brew.sh)** is recommended for installing the required tools (if they aren't already installed). To install Homebrew, from the terminal:
```
/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

**Prerequisites**
* **Xcode and its Command Line Tools**
  * **Install Xcode 9 or newer** (might work with older version, but I haven't tested it) either from the App Store or from https://developer.apple.com/downloads
  * **Install Xcode command line tools**:
    ```bash
    xcode-select --install
    ```
* **Python 2**: If not installed, see [this](https://docs.python-guide.org/starting/install/osx/) for instructions on how to install Python 2 with Homebrew.
* **CMake**: To install using Homebrew:
  ```bash
  brew install cmake
  ```
* **Clone this repo** using git (or download it using "Download ZIP" on github). From the terminal:
  ```bash
  git clone https://github.com/mceSystems/node-jsc
  cd node-jsc
  ``` 
* **Configure Xcode for WebKit**, from the terminal in node-jsc root directory:
  ```bash
  sudo deps/jscshim/webkit/Tools/Scripts/configure-xcode-for-ios-development
  ```

**Building for iOS**  
To build node-jsc, use `build_ios.sh` from the root directory. Before using it for the first time, make sure the script is executable:
```bash
chmod +x build_ios.sh
````
then:
```bash
./build_ios.sh
```

Which will build:
* Webkit (JavaScriptCore and WTF only), node.js and the rest of its dependecies (including jscshim) as static libraries.
* **NodeIOS iOS framework**, which bundles all of the compiled static libraries and provides a simple function for running node (in NodeIOS.h):
  ```c
  int node_start(int argc, char *argv[]);
  ```
  ***It is highly recommended to use the NodeIOS framework, rather than using the libraries directly.***

### Building on Windows

**Prerequisites**
  * Follow **"Installing Development Tools" from [Webkit's Windows build instructions](https://webkit.org/webkit-on-windows/), steps 1-9**.

**Building**  
From the command line, at the node-jsc root directory, use vcbuild.bat to generate a Visual Studio solution and build it:
```
vcbuild.bat vs2017 x64 nosnapshot noetw noperfctr jsc
```
When the build process is finished, node.exe will be found under the "Release" directory.

Note that this will run WebKit's build script, which will download additional support libraries for WebKit the first time it is used.

### Building on\for Other Platforms
While node-jsc haven't been tested on other platforms yet, as long as a platform is supported by both node and WebKit, node-jsc should support it, or should be able to support it with some platform specific fixes.
If you try to build\run node-jsc on another platform - let us know if it works, and feel free to open an issue about it (or even better - a pull request) if needed.

## Using node-jsc in your app
It is recommended to use the iOS framework (built by `build_ios.sh`), found at tools/NodeIOS/Release-iphoneos:
1) Add the framework to your Xcode project.
2) Add your node.js source files, in a directory, to you project.
3) In your native source code:
   * Objecive-C:
     * Import the NodeIOS framework:
       ```objectivec
       #import <NodeIOS/NodeIOS.h>
       ```
     * Locate your entry point script (index.js, for example, or loader.js in the test project), and run node. For example, assuming your entry point is loader.js under a directory called "js" (bundeled with your project):
        ```objectivec
        NSString * entryPointFilePath = [[NSBundle mainBundle] pathForResource:@"loader" ofType:@"js" inDirectory: @"js"];
        const char * nodeArgs[] = { "node", [loaderFilePath UTF8String] }; 
        node_start(2, nodeArgs);
        ```
        Note that node_start is blocking, so it is recommended to run it in its own thread (see [NodeIOS Demo Project](https://github.com/mceSystems/NodeIOS-Demo-Project) for an example).
4) Since the current directory is not the js sources directory, it is recommended to change the current dir from the javascript code. For example:
  ```javascript
    process.chdir(__dirname);
  ```

See [NodeIOS Demo Project](https://github.com/mceSystems/NodeIOS-Demo-Project) for an example project, which:
* Uses the NodeIOS framework
* Bundles a sample javascript application
  * All javascript related files are bundled in a separate folder
  * A basic "loader" (loader.js) is used as a simple entry point:
    ```javascript
    process.chdir(__dirname + "/app");
    require("./app");
    ```
    While the javascript app itself is under the js/app directory
* Redirects the console into a TextView
* Runs node in a new thread

## Native Modules
Native modules are supported, through cross compiling with node-gyp. See [node-native-script](https://github.com/mceSystems/node-native-script) for an example.
Prerequisites:
* Cloned node-jsc repo.
* On iOS, executables must by signed, so a valid, configured, code signing identity is needed. To view configured identities:
  ```
  security find-identity -v -p codesigning
  ```
  For more information, see ["inside-code-signing"](https://www.objc.io/issues/17-security/inside-code-signing/).

To build your native extension project for arm64, from the terminal:
1. Compile the extension ("\<node-jsc dir\>" is the path of your local node-jsc repo):
    ```bash
    <node-jsc dir>/deps/npm/node_modules/node-gyp/bin/node-gyp.js configure --nodedir=<node-jsc dir> --arch=arm64 --OS=ios --node_engine=jsc
    <node-jsc dir>/deps/npm/node_modules/node-gyp/bin/node-gyp.js build --nodedir=<node-jsc dir> --arch=arm64 --OS=ios --node_engine=jsc
    ```
2. Sign the extension's executable file:
    ```bash
    codesign --sign "<code signing identity>" --force ./build/Release/<Your project name>.node
    ```
3. Copy the project directory (possibly omitting unneeded files like c\c++ source files, documentation files, etc.) to your app's javascript directory.

See [node-native-script's build script](https://github.com/mceSystems/node-native-script/blob/master/build_ios.sh) for an example on how to bundle the steps above into a bash script.

## What's Missing\TODO
As node-jsc is experminetal and still an early proof of concept, a lot is still missing:
- **Inspector\debugger support**: Debugging support is obviously critical, and is the next big thing I'll work on.
  - A possible course of action will be to intergrate [NativeScript](https://www.nativescript.org)'s [iOS Runtime debugging support](https://docs.nativescript.org/tooling/debugging/debugging), which
    seems great. On iOS, NativeScript's [iOS Runtime](https://docs.nativescript.org/core-concepts/ios-runtime/Overview) is based on JavaScriptCore, and supports debugging with either WebKit's inspector or Chrome DevTools.
- iOS simulator support (should be mainly build script\gyp file related changes)
- Update the code base to the latest node v10
- Implement more missing v8 APIs
- Tests:
  - Run node.js unit tests
  - Update v8 unit tests (and pass more)

## Other Projects, Credits and Contributions
As node-jsc isn't the first project trying to use another javascript engine or support iOS, the great work done in other projects really helped node-jsc's development:
- [node-chakracore](https://github.com/nodejs/node-chakracore) was used as a reference project, using ideas and actual pieces of code when possible.
- [nodejs-mobile](https://github.com/janeasystems/nodejs-mobile), a node-chakracore fork targeting Android and iOS, has done great job in preparing node.js to be compiled and used on iOS. Parts of node-jsc's build process and gyp configuration were taken from nodejs-mobile, while nodejs-mobile's patches to libuv, node-gyp and gyp were merged into node-jsc to help everything compile for iOS and to support native extensions. 
- [spidernode](https://github.com/mozilla/spidernode) and [SpiderMonkey](https://github.com/mozilla/spidernode/tree/master/deps/spidershim/spidermonkey), also used as a reference, while existing code was taken when possible.
- NativeScript's [iOS Runtime](https://docs.nativescript.org/core-concepts/ios-runtime/Overview) was used as a reference for compiling WebKit for iOS using CMake.

## Further Reading
* [jscshim's documentation](https://github.com/mceSystems/node-jsc/blob/master/deps/jscshim/docs/jscshim.md).
* [jscshim status and known issues](https://github.com/mceSystems/node-jsc/blob/master/deps/jscshim/docs/jscshim_status_and_known_issues.md).
* node-jsc's [WebKit fork and compilation](https://github.com/mceSystems/node-jsc/blob/master/deps/jscshim/docs/webkit_fork_and_compilation.md).
* [node-native-script](https://github.com/mceSystems/node-native-script).

## License
See the LICENSE file at the node-jsc's root directory.