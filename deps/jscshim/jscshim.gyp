# This file is licensed under the terms found in the LICENSE file in 
# node-jsc's root directory.

{
  'variables': {
    'jscshim_jsc_types_header_generator_exec': '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)jscshim_jsc_types_generator<(EXECUTABLE_SUFFIX)',
    'dump_build_config_gyp_script_path': 'test/tools/testrunner/utils/dump_build_config_gyp.py'   
  },

  'includes': [
    'webkit.gypi',
  ],
  
  'target_defaults': {
    'include_dirs': [
      './include/jsc',
      '../icu-small/source/i18n',
      '../icu-small/source/common'
    ],
  },

  'targets': [
   {
    'target_name': 'jscshim_jsc_types_generator',
    'type': 'executable',

    'dependencies': [
      'webkit.gyp:jsc',
    ],

    'conditions': [
      ['v8_enable_i18n_support==1', {
        'dependencies': [
          '<(icu_gyp_path):icui18n',
          '<(icu_gyp_path):icuuc',
        ],
      }],
      ['OS in "linux"', {
        'cflags': [ '-Wno-expansion-to-defined' ],
        'cflags_cc': [ '-fexceptions', '-std=gnu++14' ],
      }],
      ['OS in "mac ios"', {
        'cflags_cc': [ '-fexceptions', '-std=gnu++14' ],
        'cflags_cc!': [ '-fno-exceptions' ],
        'xcode_settings': {
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
          'OTHER_CFLAGS': [ '-fexceptions', '-std=gnu++14' ],
        },
      }],
    ],

    'sources': [
      'jscshim_jsc_types_generator/jscshim_jsc_types_generator.cpp',
      'jscshim_jsc_types_generator/util.h'
    ],
   },

  # We have to define the script execution in a separate target as it seems gyp doesn't have
  # a proper, cross platform, "post build" actions\steps.
   {
    'target_name': 'jscshim_jsc_types_header_generator',
    'type': 'none',
    'toolsets': ['host'],
    'dependencies': [
        'jscshim_jsc_types_generator#target'
    ],

    'actions': [
     {
      'action_name': 'jscshim_jsc_types_header_generator',
      'inputs': [
        '<(jscshim_jsc_types_header_generator_exec)'
      ],
      'outputs': [
        '<(SHARED_INTERMEDIATE_DIR)/jsc-types.h',
      ],
      'action': ['python', 'jscshim_jsc_types_generator/generate_jsc_types_header.py', '<@(_inputs)', '<@(_outputs)'],
     }
    ],
   },

    {
    'target_name': 'jscshim',
    'type': 'static_library',
    'dependencies': [
      'webkit.gyp:jsc',
      'jscshim_jsc_types_header_generator#host'
    ],

    'defines': [
      'BUILDING_V8_SHARED=1',
      'BUILDING_V8_PLATFORM_SHARED=1',
      'BUILDING_JSCSHIM=1'
    ],

    'include_dirs': [
      './include',
      '.',
    ],

    # Note that "direct dependent" is not enough, since node is devided into multiple, dependent
    # targets: node's static_node target (which contains node_main.cc, which uses v8.h) depends
    # on "node" target (defined by "node_core_target_name"), which depends on jscshim.
    'all_dependent_settings': {
      'include_dirs': [
        './include',
        '<(SHARED_INTERMEDIATE_DIR)', # For jsc-types.h
      ],
    },

    'conditions': [
      ['v8_promise_internal_field_count!=0', {
        'defines': ['JSCSHIM_PROMISE_INTERNAL_FIELD_COUNT=<(v8_promise_internal_field_count)'],
      }],
      ['OS=="ios"', {
        'defines': [
          'JSCSHIM_DISABLE_JIT=1'
        ]
      }],
      # fexceptions related flags were taken from node-mobile (chakrashim)
      ['OS=="win"', {
        'defines': [
          'NOMINMAX'
        ]
      }],
      ['OS in "linux"', {
        'cflags': [ '-Wno-expansion-to-defined' ],
        'cflags_cc': [ '-fexceptions', '-std=gnu++14' ],
        'link_settings': {
          'libraries': [ '-ldl' ],
        },
      }],
      ['OS in "mac ios"', {
        'cflags_cc': [ '-fexceptions', '-std=gnu++14' ],
        'cflags_cc!': [ '-fno-exceptions' ],
        'xcode_settings': {
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
          'OTHER_CFLAGS': [ '-fexceptions', '-std=gnu++14' ],
        },
      }],
    ],

    # TODO: Add inspector headers
    'sources': [
      'include/jsc-value.h',
      'include/v8-debug.h',
      'include/v8-inspector.h',
      'include/v8-platform.h',
      'include/v8-profiler.h',
      'include/v8.h',
      'include/v8config.h',

      'include/libplatform/libplatform-export.h',
      'include/libplatform/libplatform.h',
      'include/libplatform/v8-tracing.h',

      'src/Debug.cpp',
      'src/v8.cpp',
      'src/v8AccessorSignature.cpp',
      'src/v8Array.cpp',
      'src/v8ArrayBuffer.cpp',
      'src/v8Boolean.cpp',
      'src/v8BooleanObject.cpp',
      'src/v8Context.cpp',
      'src/v8DataView.cpp',
      'src/v8Date.cpp',
      'src/v8Exception.cpp',
      'src/v8External.cpp',
      'src/v8Function.cpp',
      'src/v8FunctionTemplate.cpp',
      'src/v8globals.cpp',
      'src/v8Integer.cpp',
      'src/v8Isolate.cpp',
      'src/v8json.cpp',
      'src/v8Map.cpp',
      'src/v8Message.cpp',
      'src/v8Module.cpp',
      'src/v8Number.cpp',
      'src/v8NumberObject.cpp',
      'src/v8Object.cpp',
      'src/v8ObjectTemplate.cpp',
      'src/v8platform.cpp',
      'src/v8Private.cpp',
      'src/v8profiler.cpp',
      'src/v8Promise.cpp',
      'src/v8PropertyDescriptor.cpp',
      'src/v8Proxy.cpp',
      'src/v8RegExp.cpp',
      'src/v8Script.cpp',
      'src/v8Set.cpp',
      'src/v8SharedArrayBuffer.cpp',
      'src/v8Signature.cpp',
      'src/v8StackTrace.cpp',
      'src/v8String.cpp',
      'src/v8StringObject.cpp',
      'src/v8Symbol.cpp',
      'src/v8SymbolObject.cpp',
      'src/v8Template.cpp',
      'src/v8tracing.cpp',
      'src/v8TryCatch.cpp',
      'src/v8TypedArrays.cpp',
      'src/v8Value.cpp',
      'src/v8Value.h',
      'src/v8ValueDeserializer.cpp',
      'src/v8ValueSerializer.cpp',
      'src/v8WeakWrapper.cpp',

      'src/shim/APIAccessor.cpp',
      'src/shim/APIAccessor.h',
      'src/shim/ArrayBufferHelpers.h',
      'src/shim/CallSite.cpp',
      'src/shim/CallSite.h',
      'src/shim/CallSitePrototype.cpp',
      'src/shim/CallSitePrototype.h',
      'src/shim/EmbeddedFieldsContainer.h',
      'src/shim/exceptions.h',
      'src/shim/External.cpp',
      'src/shim/External.h',
      'src/shim/Function.cpp',
      'src/shim/Function.h',
      'src/shim/FunctionTemplate.cpp',
      'src/shim/FunctionTemplate.h',
      'src/shim/GlobalObject.cpp',
      'src/shim/GlobalObject.h',
      'src/shim/GlobalObjectInlines.h',
      'src/shim/helpers.h',
      'src/shim/InterceptorInfo.h',
      'src/shim/Isolate.cpp',
      'src/shim/Isolate.h',
      'src/shim/JSCStackTrace.cpp',
      'src/shim/JSCStackTrace.h',
      'src/shim/Message.cpp',
      'src/shim/Message.h',
      'src/shim/Object.cpp',
      'src/shim/Object.h',
      'src/shim/ObjectTemplate.cpp',
      'src/shim/ObjectTemplate.h',
      'src/shim/ObjectWithInterceptors.cpp',
      'src/shim/ObjectWithInterceptors.h',
      'src/shim/PromiseResolver.cpp',
      'src/shim/PromiseResolver.h',
      'src/shim/Script.cpp',
      'src/shim/Script.h',
      'src/shim/StackFrame.cpp',
      'src/shim/StackFrame.h',
      'src/shim/StackTrace.cpp',
      'src/shim/StackTrace.h',
      'src/shim/Template.cpp',
      'src/shim/Template.h',
      'src/shim/TemplateProperty.cpp',
      'src/shim/TemplateProperty.h',

      'src/internal/Heap.cpp',

      'src/base/base-export.h',
      'src/base/build_config.h',
      'src/base/compiler-specific.h',
      'src/base/logging.cc',
      'src/base/logging.h',
      'src/base/macros.h',
      'src/base/platform/platform.h',

      'src/inspector/string-16.cc',
      'src/inspector/string-16.h',
      'src/inspector/string-util.cc',
      'src/inspector/string-util.h',
      'src/inspector/V8Inspector.cpp',
      'src/inspector/V8InspectorImpl.cpp',
      'src/inspector/V8InspectorImpl.h',
      'src/inspector/V8InspectorSessionImpl.cpp',
      'src/inspector/V8InspectorSessionImpl.h',
      'src/inspector/V8StackStraceImpl.cpp',
      'src/inspector/V8StackStraceImpl.h',
      'src/inspector/auto-generated/Console.cpp',
      'src/inspector/auto-generated/Console.h',
      'src/inspector/auto-generated/Debugger.cpp',
      'src/inspector/auto-generated/Debugger.h',
      'src/inspector/auto-generated/Forward.h',
      'src/inspector/auto-generated/HeapProfiler.cpp',
      'src/inspector/auto-generated/HeapProfiler.h',
      'src/inspector/auto-generated/Profiler.cpp',
      'src/inspector/auto-generated/Profiler.h',
      'src/inspector/auto-generated/Protocol.cpp',
      'src/inspector/auto-generated/Protocol.h',
      'src/inspector/auto-generated/Runtime.cpp',
      'src/inspector/auto-generated/Runtime.h',
      'src/inspector/auto-generated/Schema.cpp',
      'src/inspector/auto-generated/Schema.h'
    ]
   },

   # Based on the v8_dump_build_config target from v8.gyp
   {
    'target_name': 'jscshim_dump_testrunner_build_config',
    'type': 'none',
    'variables': {
    },
    'actions': [
      {
        'action_name': 'jscshim_dump_testrunner_build_config',
        'inputs': [
          '<(dump_build_config_gyp_script_path)',
        ],
        'outputs': [
          '<(PRODUCT_DIR)/v8_build_config.json',
        ],
        'action': [
          'python',
          '<(dump_build_config_gyp_script_path)',
          '<(PRODUCT_DIR)/v8_build_config.json',
          'dcheck_always_on=0',
          'is_asan=0',
          'is_cfi=0',
          'is_component_build=0',
          'is_debug=<(jsc_build_config)',
          # Not available in gyp.
          'is_gcov_coverage=0',
          'is_msan=0',
          'is_tsan=0',
          # Not available in gyp.
          'is_ubsan_vptr=0',
          'target_cpu=<(target_arch)',
          'v8_enable_i18n_support=<(v8_enable_i18n_support)',
          'v8_enable_verify_predictable=0',
          'v8_target_cpu=<(target_arch)',
          'v8_use_snapshot=0',
        ],
        'conditions': [
          ['target_arch=="mips" or target_arch=="mipsel" \
            or target_arch=="mips64" or target_arch=="mips64el"', {
              'action':[
                'mips_arch_variant=<(mips_arch_variant)',
                'mips_use_msa=<(mips_use_msa)',
              ],
          }],
        ],
      },
    ],
   },

   {
    'target_name': 'jscshim_tests',
    'dependencies': [
      'jscshim',
      'jscshim_dump_testrunner_build_config#target',
      'webkit.gyp:jsc',
    ],

    'include_dirs': [
      '.',
      './include',
      './src',
      './test',
      './test/src'
    ],

    'link_settings': {
      'libraries': [ '<@(webkit_output_libraries)' ],
    },

    'conditions': [
      # fexceptions related flags were taken from node-mobile (chakrashim)
      ['OS=="win"', {
        'defines': [
          'NOMINMAX'
        ]
      }],
      ['OS in "linux"', {
        'cflags': [ '-Wno-expansion-to-defined' ],
        'cflags_cc': [ '-fexceptions', '-std=gnu++14' ],
        'link_settings': {
          'libraries': [ '-ldl' ],
        },
      }],
      ['OS in "mac ios"', {
        'cflags_cc': [ '-fexceptions', '-std=gnu++14' ],
        'cflags_cc!': [ '-fno-exceptions' ],
        'xcode_settings': {
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
          'OTHER_CFLAGS': [ '-fexceptions', '-std=gnu++14' ],
        },
      }],

      ['v8_enable_i18n_support==1', {
        'dependencies': [
          '<(icu_gyp_path):icui18n',
          '<(icu_gyp_path):icuuc',
        ],
      }],

      ['OS=="ios"', {
        'type': 'static_library',
        'defines': [
          'CCTEST_STATIC'
        ]
      }, {
        'type': 'executable',
      }]
    ],

    'sources': [
      'test/src/cctest.cc',
      'test/src/cctest.h',
      'test/src/test-api.cc',
      'test/src/test-api.h',
      'test/src/v8-pollyfill.h',

      'test/src/jscutshim/internal.cpp',
      'test/src/jscutshim/internal.h',
      'test/src/jscutshim/test-helpers.cpp',
      'test/src/jscutshim/test-helpers.h',

      'test/src/testing/gtest/include/gtest/gtest_prod.h',

      'test/src/v8/allocation.cc',
      'test/src/v8/allocation.h',
      'test/src/v8/checks.h',
      'test/src/v8/globals.h',
      'test/src/v8/list.h',
      'test/src/v8/splay-tree.h',
      'test/src/v8/utils.h',
      'test/src/v8/v8.h',
      'test/src/v8/vector.h',

      'test/src/v8/base/atomic-utils.h',
      'test/src/v8/base/atomicops.h',
      'test/src/v8/base/atomicops_internals_atomicword_compat.h',
      'test/src/v8/base/atomicops_internals_portable.h',
      'test/src/v8/base/atomicops_internals_x86_msvc.h',
      'test/src/v8/base/base-export.h',
      'test/src/v8/base/bits.cc',
      'test/src/v8/base/bits.h',
      'test/src/v8/base/cpu.cc',
      'test/src/v8/base/cpu.h',
      'test/src/v8/base/flags.h',
      'test/src/v8/base/hashmap-entry.h',
      'test/src/v8/base/hashmap.h',
      'test/src/v8/base/lazy-instance.h',
      'test/src/v8/base/once.cc',
      'test/src/v8/base/once.h',
      'test/src/v8/base/safe_conversions.h',
      'test/src/v8/base/safe_conversions_impl.h',
      'test/src/v8/base/safe_math.h',
      'test/src/v8/base/safe_math_impl.h',
      'test/src/v8/base/win32-headers.h',
      'test/src/v8/base/platform/elapsed-timer.h',
      'test/src/v8/base/platform/mutex.cc',
      'test/src/v8/base/platform/mutex.h',
      'test/src/v8/base/platform/semaphore.cc',
      'test/src/v8/base/platform/semaphore.h',
      'test/src/v8/base/platform/time.cc',
      'test/src/v8/base/platform/time.h',
      'test/src/v8/base/utils/random-number-generator.cc',
      'test/src/v8/base/utils/random-number-generator.h',

      'test/src/v8/zone/accounting-allocator.cc',
      'test/src/v8/zone/accounting-allocator.h',
      'test/src/v8/zone/zone-segment.cc',
      'test/src/v8/zone/zone-segment.h',
      'test/src/v8/zone/zone.h',
    ]
   }
  ]
}
