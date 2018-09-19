# This file is licensed under the terms found in the LICENSE file in 
# node-jsc's root directory.

{
  'variables': {
    'conditions': [
      ['OS=="ios"', {
        'webkit_output_dir': './webkit/WebKitBuild/',
        'webkit_output_libraries': ['<!(pwd)/webkit/WebKitBuild/lib/<(jsc_build_config)/<(STATIC_LIB_PREFIX)bmalloc<(STATIC_LIB_SUFFIX)', 
                                    '<!(pwd)/webkit/WebKitBuild/lib/<(jsc_build_config)/<(STATIC_LIB_PREFIX)WTF<(STATIC_LIB_SUFFIX)', 
                                    '<!(pwd)/webkit/WebKitBuild/lib/<(jsc_build_config)/<(STATIC_LIB_PREFIX)JavaScriptCore<(STATIC_LIB_SUFFIX)'],
      }, {
        'webkit_output_dir': './webkit/WebKitBuild/<(jsc_build_config)',
        'webkit_output_libraries': ['<!(python -c "import os; print os.getcwd()")/webkit/WebKitBuild/<(jsc_build_config)/lib/<(STATIC_LIB_PREFIX)WTF<(STATIC_LIB_SUFFIX)',
                                    '<!(python -c "import os; print os.getcwd()")/webkit/WebKitBuild/<(jsc_build_config)/lib/<(STATIC_LIB_PREFIX)JavaScriptCore<(STATIC_LIB_SUFFIX)'],
      }],
    ]
  }
}
