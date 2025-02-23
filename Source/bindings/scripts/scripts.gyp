# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Pre-caching steps used internally by the IDL compiler
#
# Design doc: http://www.chromium.org/developers/design-documents/idl-build

{
  'includes': [
    'scripts.gypi',
    '../bindings.gypi',
    '../templates/templates.gypi',
  ],

  'targets': [
################################################################################
  {
    # A separate pre-caching step is *not required* to use lex/parse table
    # caching in PLY, as the caches are concurrency-safe.
    # However, pre-caching ensures that all compiler processes use the cached
    # files (hence maximizing speed), instead of early processes building the
    # tables themselves (as they've not yet been written when they start).
    'target_name': 'cached_lex_yacc_tables',
    'type': 'none',
    'actions': [{
      'action_name': 'cache_lex_yacc_tables',
      'inputs': [
        '<@(idl_lexer_parser_files)',
      ],
      'outputs': [
        '<(bindings_scripts_output_dir)/lextab.py',
        '<(bindings_scripts_output_dir)/parsetab.pickle',
      ],
      'action': [
        'python',
        'blink_idl_parser.py',
        '<(bindings_scripts_output_dir)',
      ],
      'message': 'Caching PLY lex & yacc lex/parse tables',
    }],
  },
################################################################################
  {
    # A separate pre-caching step is *required* to use bytecode caching in
    # Jinja (which improves speed significantly), as the bytecode cache is
    # not concurrency-safe on write; details in code_generator_v8.py.
    'target_name': 'cached_jinja_templates',
    'type': 'none',
    'actions': [{
      'action_name': 'cache_jinja_templates',
      'inputs': [
        '<@(jinja_module_files)',
        'code_generator_v8.py',
        '<@(code_generator_template_files)',
      ],
      'outputs': [
        '<(bindings_scripts_output_dir)/cached_jinja_templates.stamp',  # Dummy to track dependency
      ],
      'action': [
        'python',
        'code_generator_v8.py',
        '<(bindings_scripts_output_dir)',
        '<(bindings_scripts_output_dir)/cached_jinja_templates.stamp',
      ],
      'message': 'Caching bytecode of Jinja templates',
    }],
  },
################################################################################
  ],  # targets
}
