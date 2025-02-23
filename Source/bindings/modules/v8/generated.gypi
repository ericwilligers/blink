# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'includes': [
    '../../bindings.gypi',
  ],
  'variables': {
    'bindings_modules_v8_output_dir': '<(bindings_output_dir)/modules/v8',

    'conditions': [
      ['OS=="win" and buildtype=="Official"', {
        # On Windows Official release builds, we try to preserve symbol
        # space.
        'bindings_modules_generated_aggregate_files': [
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings.cpp',
        ],
      }, {
        'bindings_modules_generated_aggregate_files': [
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings01.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings02.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings03.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings04.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings05.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings06.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings07.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings08.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings09.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings10.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings11.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings12.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings13.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings14.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings15.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings16.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings17.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings18.cpp',
          '<(bindings_modules_v8_output_dir)/V8GeneratedModulesBindings19.cpp',
        ],
      }],
    ],
  },
}
