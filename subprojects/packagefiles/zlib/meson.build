project('zlib', 'c',
  version: '1.3.2',
  default_options: ['default_library=static'],
)

zlib_sources = files(
  'adler32.c',
  'compress.c',
  'crc32.c',
  'deflate.c',
  'gzclose.c',
  'gzlib.c',
  'gzread.c',
  'gzwrite.c',
  'infback.c',
  'inffast.c',
  'inflate.c',
  'inftrees.c',
  'trees.c',
  'uncompr.c',
  'zutil.c',
)

zlib_inc = include_directories('.')
zlib_c_args = []
if host_machine.system() == 'windows'
  zlib_c_args += [
    '-D_CRT_SECURE_NO_WARNINGS',
    '-D_CRT_NONSTDC_NO_WARNINGS',
  ]
endif

zlib_dep = declare_dependency(
  include_directories: zlib_inc,
  compile_args: zlib_c_args,
  sources: zlib_sources,
)

meson.override_dependency('zlib', zlib_dep)
