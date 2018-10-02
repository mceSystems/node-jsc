""""
This source code is licensed under the terms found in the LICENSE file in 
node-jsc's root directory.
"""

# TODO Android support (use https://github.com/SoftwareMansion/jsc-android-buildscripts?)
# TODO: Find a better solution for ICU lib paths than the --build-static-lib-prefix\--build-static-lib-suffix args

import optparse
import os
import sys

# Taken from node's configure script, without "android". Note that WebKit doesn't necessarily suuport all of them
VALID_OS = ('win', 'mac', 'solaris', 'freebsd', 'openbsd', 'linux',
            'ios', 'aix')
VALID_ARCH = ('arm', 'arm64', 'ia32', 'mips', 'mipsel', 'mips64el', 'ppc',
              'ppc64', 'x32','x64', 'x86', 's390', 's390x')

# Ideally, we should only disable WebAssembly on iOS (where you can't
# allocate executable memory), but it also seem not to compile on Windows
# (JSC uses mrpotect).
OS_WITHOUT_WEBASSEMBLY_SUPPORT = ('ios', 'win')

ICU_SUBDIRS = ('source/common', 'source/i18n')
ICU_LIBS = ('icuucx', 'icustubdata', 'icui18n', 'icutools')

# Taken from node's tools/icu/icu-generic.gyp ("icu_uconfig" target)
# TODO: This might not work for user provided ICUs, but for now we'll foccus on node's ICU
ICU_FLAGS = '-DUCONFIG_NO_SERVICE=1;-DUCONFIG_NO_REGULAR_EXPRESSIONS=1;-DU_ENABLE_DYLOAD=0;-DU_STATIC_IMPLEMENTATION=1;-DU_HAVE_STD_STRING=0;-DUCONFIG_NO_BREAK_ITERATION=0'

# When running cmake from gyp, cmake failed with various errors.
# Deleting the environment variables created by gyp seem to fix it.
IOS_ENV_VARS_TO_DELETE = ('BUILT_FRAMEWORKS_DIR', 'BUILT_PRODUCTS_DIR', 'PRODUCT_NAME', 'MAKEFLAGS', 'BUILDTYPE', 'TARGET_BUILD_DIR', 'XCODE_VERSION_ACTUAL', 'CONFIGURATION', 'MFLAGS', 'SRCROOT', 'V', 'MAKEOVERRIDES', 'SOURCE_ROOT', 'MAKELEVEL', 'SDKROOT')

CURRENT_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

IOS_TOOLCHAIN_PATH = os.path.join(CURRENT_SCRIPT_DIR, 'ios.toolchain.cmake')
IOS_CMAKE_COMMON_ARGS = (
	# General cmake
	'-GXcode',
	'-DCMAKE_TOOLCHAIN_FILE={}'.format(IOS_TOOLCHAIN_PATH),
	'-DDEVELOPER_MODE=ON',

	# JSC
	'-DPORT=JSCOnly',
	'-DENABLE_STATIC_JSC=1',
	'-DENABLE_JIT=ON',
	'-DENABLE_FTL_JIT=OFF',
	'-DENABLE_INSPECTOR=ON',
	'-DENABLE_REMOTE_INSPECTOR=OFF',
	'-DENABLE_RESOURCE_USAGE=ON',
	'-DENABLE_INTL=OFF',
	'-DENABLE_WEBCORE=OFF',
	'-DENABLE_WEBKIT=OFF',
	'-DENABLE_WEBKIT_LEGACY=OFF',
	'-DENABLE_API_TESTS=OFF'
)

IOS_XCODE_BUILD_COMMAND = 'xcodebuild -target JavaScriptCore -sdk iphoneos -configuration {} ARCHS=arm64 ONLY_ACTIVE_ARCH=NO IPHONEOS_DEPLOYMENT_TARGET="9.0" GCC_WARN_INHIBIT_ALL_WARNINGS="YES" CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO'

WEBKIT_RELATIVE_OUPUT_DIR = 'webkit/WebKitBuild'

def parse_command_line():
	# Again, based on node's configure script
	parser = optparse.OptionParser()

	parser.add_option('--node-build-output-dir',
		action='store',
		dest='node_build_output_dir',
		help='Path to node\'s output dir')

	parser.add_option('--debug',
		action='store_true',
		dest='debug',
		help='also build debug build')

	parser.add_option('--dest-cpu',
		action='store',
		dest='dest_cpu',
		choices=VALID_ARCH,
		help='CPU architecture to build for ({0})'.format(', '.join(VALID_ARCH)))

	parser.add_option('--dest-os',
		action='store',
		dest='dest_os',
		choices=VALID_OS,
		help='operating system to build for ({0})'.format(', '.join(VALID_OS)))

	parser.add_option('--icu-source-path',
		action='store',
		dest='icu_source_path',
		help='Path to ICU sources (if not passed Intl will be disabled')

	parser.add_option('--build-static-lib-prefix',
		action='store',
		dest='build_static_lib_prefix',
		help='GYP\'s STATIC_LIB_PREFIX value, used for build ICU library paths')

	parser.add_option('--build-static-lib-suffix',
		action='store',
		dest='build_static_lib_suffix',
		help='GYP\'s STATIC_LIB_SUFFIX value, used for build ICU library paths')

	(options, args) = parser.parse_args()
	return options

def create_icu_paths(options):
	icu_source_path = os.path.abspath(options.icu_source_path)
	include_dirs = [os.path.join(icu_source_path, p) for p in ICU_SUBDIRS]
	include_dirs = ';'.join(include_dirs)

	node_build_output_dir = os.path.abspath(options.node_build_output_dir)
	libraries = []
	for lib in ICU_LIBS:
		lib_relative_path = 'lib/{}{}{}'.format(options.build_static_lib_prefix, lib, options.build_static_lib_suffix)
		libraries.append(os.path.join(node_build_output_dir, lib_relative_path))
	libraries = ';'.join(libraries)

	return (include_dirs, libraries)

def configure_webkit_args_for_build_jsc(options):
	webkit_script_args = ['--cmake']
	webkit_cmake_args = []

	# Debug\Release
	if options.debug:
		webkit_script_args.append('--debug')
	else:
		webkit_script_args.append('--release')

	# arch\os related
	# TODO: Properly pass the architecture\os to cmake
	# TODO: android
	if 'ia32' == options.dest_cpu:
		webkit_script_args.append('--32-bit')
	elif 'x64' == options.dest_cpu:
		webkit_script_args.append('--64-bit')

	# Note that if we'll want to support iOS here, it seems that we'll need to set:
	# webkit_script_args.append('--ios-device')
	# webkit_cmake_args.append('-DJavaScriptCore_LIBRARY_TYPE=STATIC')
	# instead of the following two lines
	webkit_script_args.append('--jsc-only')
	webkit_cmake_args.append('-DENABLE_STATIC_JSC=ON')
	
	if 'win' == options.dest_os:
		webkit_script_args.append('--no-ninja')
		webkit_cmake_args.append('-DMSVC_STATIC_RUNTIME=ON')

	# WebAssembly 
	if options.dest_os in OS_WITHOUT_WEBASSEMBLY_SUPPORT:
		webkit_cmake_args.append('-DENABLE_WEBASSEMBLY=OFF')

	# Intl
	if options.icu_source_path:
		webkit_cmake_args.append('-DENABLE_INTL=ON')
		webkit_cmake_args.append('-DCUSTOM_ICU=1')

		# For now, instead of passing ICU_INCLUDE_DIRS\ICU_LIBRARIES directly to cmake 
		# (-DICU_INCLUDE_DIRS=..., etc.) and worry about escaping (semicolons, spaces), 
		# we'll pass it as an environment variable(which is supported by the "JSCOnly" port).
		(include_dirs, libraries) = create_icu_paths(options)
		os.environ['ICU_INCLUDE_DIRS'] = include_dirs
		os.environ['ICU_LIBRARIES'] = libraries
		os.environ['ICU_COMPILER_FLAGS'] = ICU_FLAGS
	else:
		webkit_cmake_args.append('-DENABLE_INTL=OFF')

	# Finally, add the cmake arguments to the build script's arguments
	webkit_script_args.append('--cmakeargs="{}"'.format(' '.join(webkit_cmake_args)))

	return webkit_script_args

def build_jsc_ios(options):
	os.chdir(WEBKIT_RELATIVE_OUPUT_DIR)

	for env_var in IOS_ENV_VARS_TO_DELETE:
		if env_var in os.environ:
			del os.environ[env_var]

	# Clean the build folder
	#os.system('rm -rf *')

	build_type = 'Debug' if options.debug else 'Release'

	# Run cmake to generate an Xcode project
	cmake_command = 'cmake -DCMAKE_BUILD_TYPE={} {} ..'.format(build_type, ' '.join(IOS_CMAKE_COMMON_ARGS))
	print cmake_command
	os.system(cmake_command)

	# Build the generated project(s) using xcodebuild
	build_command = IOS_XCODE_BUILD_COMMAND.format(build_type)
	os.system(build_command)
	os.system('cd DerivedSources/WTF && mig -sheader MachExceptionsServer.h MachExceptions.defs && cd ../../')
	return os.system(build_command)
		
def build_jsc(options):
	webkit_build_script_args = configure_webkit_args_for_build_jsc(options)
	
	# The build scripts use paths relative to WebKit's root the (the update scripts at least)
	# Note that this must be done right before running WebKit's script, as the above create_icu_header_paths
	# uses os.path.abspath, and relies on node's root dir to be the current dir.
	os.chdir('webkit')

	#for env_var in ENV_VARS_TO_DELETE:
	#	del os.environ[env_var]

	#webkit_build_script_args.append('--generate-project-only')

	# Note that escaping of args, if needed, is done in configure_webkit_args
	build_command = 'perl Tools/Scripts/build-jsc {}'.format(' '.join(webkit_build_script_args))
	print build_command
	sys.stdout.flush()

	return os.system(build_command)

def main():
	options = parse_command_line()
	
	if 'ios' == options.dest_os:
		return build_jsc_ios(options)

	return build_jsc(options)

if __name__== "__main__":
  sys.exit(main())