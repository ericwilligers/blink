# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Generate IDL bindings for core, plus aggregate bindings files.
#
# Design doc: http://www.chromium.org/developers/design-documents/idl-build

{
  'includes': [
    '../../../core/core.gypi',
    '../../bindings.gypi',
    # FIXME: need list of modules IDL files because some core IDL files depend
    # on modules IDL files  http://crbug.com/358074
    '../../modules/idl.gypi',
    # FIXME: need info about modules IDL files because some core IDL files
    # depend on modules IDL files  http://crbug.com/358074
    '../../modules/modules.gypi',
    '../../scripts/scripts.gypi',
    '../idl.gypi',
    'generated.gypi',
  ],

  # Write lists of main IDL files to a file, so that the command lines don't
  # exceed OS length limits.
  'variables': {
      'core_idl_files_list': '<|(core_idl_files_list.tmp <@(core_idl_files))',
  },

  'targets': [
################################################################################
  {
    'target_name': 'bindings_core_generated_individual',
    'type': 'none',
    # The 'binding' rule generates .h files, so mark as hard_dependency, per:
    # https://code.google.com/p/gyp/wiki/InputFormatReference#Linking_Dependencies
    'hard_dependency': 1,
    'dependencies': [
      '../../../core/core_generated.gyp:generated_testing_idls',
      '<(bindings_scripts_dir)/scripts.gyp:cached_jinja_templates',
      '<(bindings_scripts_dir)/scripts.gyp:cached_lex_yacc_tables',
      # FIXME: should be interfaces_info_core (w/o modules)
      # http://crbug.com/358074
      '../../modules/generated.gyp:interfaces_info',
    ],
    'sources': [
      '<@(core_interface_idl_files)',
    ],
    'rules': [{
      'rule_name': 'binding',
      'extension': 'idl',
      'msvs_external_rule': 1,
      'inputs': [
        '<@(idl_lexer_parser_files)',  # to be explicit (covered by parsetab)
        '<@(idl_compiler_files)',
        '<(bindings_scripts_output_dir)/lextab.py',
        '<(bindings_scripts_output_dir)/parsetab.pickle',
        '<(bindings_scripts_output_dir)/cached_jinja_templates.stamp',
        '<(bindings_dir)/IDLExtendedAttributes.txt',
        # If the dependency structure or public interface info (e.g.,
        # [ImplementedAs]) changes, we rebuild all files, since we're not
        # computing dependencies file-by-file in the build.
        # This data is generally stable.
        '<(bindings_modules_output_dir)/InterfacesInfoModules.pickle',
        # Further, if any dependency (partial interface or implemented
        # interface) changes, rebuild everything, since every IDL potentially
        # depends on them, because we're not computing dependencies
        # file-by-file.
        # FIXME: This is too conservative, and causes excess rebuilds:
        # compute this file-by-file.  http://crbug.com/341748
        # FIXME: should be core_all_dependency_idl_files only, but some core IDL
        # files depend on modules IDL files  http://crbug.com/358074
        '<@(all_dependency_idl_files)',
      ],
      'outputs': [
        '<(bindings_core_v8_output_dir)/V8<(RULE_INPUT_ROOT).cpp',
        '<(bindings_core_v8_output_dir)/V8<(RULE_INPUT_ROOT).h',
      ],
      # sanitize-win-build-log.sed uses a regex which matches this command
      # line (Python script + .idl file being processed).
      # Update that regex if command line changes (other than changing flags)
      'action': [
        'python',
        '-S',  # skip 'import site' to speed up startup
        '<(bindings_scripts_dir)/idl_compiler.py',
        '--cache-dir',
        '<(bindings_scripts_output_dir)',
        '--output-dir',
        '<(bindings_core_v8_output_dir)',
        '--interfaces-info',
        '<(bindings_modules_output_dir)/InterfacesInfoModules.pickle',
        '--write-file-only-if-changed',
        '<(write_file_only_if_changed)',
        '<(RULE_INPUT_PATH)',
      ],
      'message': 'Generating binding from <(RULE_INPUT_PATH)',
    }],
  },
################################################################################
  {
    'target_name': 'bindings_core_generated_aggregate',
    'type': 'none',
    'actions': [{
      'action_name': 'generate_aggregate_bindings_core',
      'inputs': [
        '<(bindings_scripts_dir)/aggregate_generated_bindings.py',
        '<(core_idl_files_list)',
      ],
      'outputs': [
        '<@(bindings_core_generated_aggregate_files)',
      ],
      'action': [
        'python',
        '<(bindings_scripts_dir)/aggregate_generated_bindings.py',
        'core',
        '<(core_idl_files_list)',
        '--',
        '<@(bindings_core_generated_aggregate_files)',
      ],
      'message': 'Generating aggregate generated core bindings files',
    }],
  },
################################################################################
  {
    'target_name': 'bindings_core_generated',
    'type': 'none',
    'dependencies': [
      'bindings_core_generated_aggregate',
      'bindings_core_generated_individual',
    ],
  },
################################################################################
  ],  # targets
}
