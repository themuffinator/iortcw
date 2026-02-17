project('opusfile', 'c',
  version: '0.12',
  default_options: ['default_library=static'],
)

ogg_dep = dependency('ogg', required: true)
opus_dep = dependency('opus', required: true)

opusfile_inc = include_directories('include')

opusfile_dep = declare_dependency(
  sources: [
    'src/info.c',
    'src/internal.c',
    'src/opusfile.c',
    'src/stream.c',
  ],
  include_directories: opusfile_inc,
  dependencies: [ogg_dep, opus_dep],
)

meson.override_dependency('opusfile', opusfile_dep)
