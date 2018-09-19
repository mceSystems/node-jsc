# This file is licensed under the terms found in the LICENSE file in 
# node-jsc's root directory.

{
  'variables': {
    # Note that the dot at the end of --node-build-output-dir is there to prevent 
    # the path from exapnding to a path ending with "\", which messes the command line on windows
    'build_jsc_args': ['--node-build-output-dir=<(PRODUCT_DIR).', 
                       '--dest-cpu=<(target_arch)', 
                       '--dest-os=<(OS)',
                       '--build-static-lib-prefix=<(STATIC_LIB_PREFIX)',
                       '--build-static-lib-suffix=<(STATIC_LIB_SUFFIX)'],

	  'conditions': [
      ['jsc_build_config=="Debug"', {
        'build_jsc_args+': ['--debug'],
      }],

      # intl
      ['v8_enable_i18n_support==1', {
        'build_jsc_args': ['--icu-source-path=<(icu_path)'],
      }],  
	  ]
  },

  'includes': [
    'webkit.gypi',
  ],

  'targets': [
   {
    'target_name': 'jsc',
    'toolsets': ['target'],
    'type': 'none',

    'all_dependent_settings': {
      # TODO: Move to direct_dependent_settings?
      'include_dirs': [
        '<(webkit_output_dir)/DerivedSources/ForwardingHeaders',
        '<(webkit_output_dir)/DerivedSources/ForwardingHeaders/JavaScriptCore',
        './WebKit/Source/bmalloc',

        # For cmakeconfig.h
        '<(webkit_output_dir)',

      ],
      'link_settings': {
        'libraries': ['<@(webkit_output_libraries)']
      },
      'conditions': [
        # Internally JSC\WTF will always use ICU, so use the default one if we're not using node's
        ['v8_enable_i18n_support==0 and (OS=="mac" or OS=="ios")', {
          'link_settings': {
            'libraries': ['-licucore']
          },
        }],
        ['OS=="mac" or OS=="ios"', {
          'link_settings': {
            'libraries': ['-framework Foundation']
          },
        }],
      ],
    },

    'direct_dependent_settings': {
      'defines': [
        'STATICALLY_LINKED_WITH_JavaScriptCore=1',
        'STATICALLY_LINKED_WITH_WTF=1',
        'HAVE_CONFIG_H=1',
        'BUILDING_WITH_CMAKE=1',
        'NOMINMAX',
        'ENABLE_INSPECTOR_ALTERNATE_DISPATCHERS=0',
        'ENABLE_RESOURCE_USAGE=1',

        # Needed for platform checks in wtf/Platform.h, or else jscshim will get WTF_PLATFORM_IOS defined, 
        # which will cause inline (in header) code to behave differently when included in jschim.
        'BUILDING_JSCONLY__',
      ],
      'conditions': [
        ['OS=="mac" or OS=="ios"', {
          'defines': [
            'USE_FOUNDATION=1',
            'USE_CF_RETAIN_PTR=1'
          ],
        }],
        ['OS=="win"', {
          'msvs_settings': {
            'VCCLCompilerTool': {
              'AdditionalOptions': [
                '/std:c++17'
              ],
            }
          }
        }],
        ['clang==1', {
          'xcode_settings': {
            'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0',
            'CLANG_CXX_LANGUAGE_STANDARD': 'gnu++17',
            'CLANG_CXX_LIBRARY': 'libc++',
          },
        }],
        ['v8_enable_i18n_support==0 and (OS=="mac" or OS=="ios")', {
          # TODO: It seems the WTF chooses uint16_t for UChar for some reason. 
          # Are we using different ICU headers?
          'defines': ['UCHAR_TYPE=uint16_t'],
        }],
      ]
    },

    'conditions': [
      ['v8_enable_i18n_support==1', {
        'dependencies': [
          '<(icu_gyp_path):icui18n',
          '<(icu_gyp_path):icuuc',
        ],

        'conditions': [
          ['OS=="win"', {
          'all_dependent_settings': {
            'link_settings': {
              'libraries': [ '-lWinmm.lib' ],
            },
          },
        }]
        ],
      }],
    ],

    'actions': [
     {
      'action_name': 'build_webkit',
      'inputs': [
        'tools/build_jsc.py',

        # Use the ChangeLogs as inputs since they change when WebKit updates, which will trigger this action
        'WebKit/Source/JavaScriptCore/ChangeLog',
        'WebKit/Source/WTF/ChangeLog'
      ],
      'outputs': ['<@(webkit_output_libraries)'],
      'action': ['python', 'tools/build_jsc.py', '<@(build_jsc_args)']
     }
    ]
   },
  ],
}