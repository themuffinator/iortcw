project('iortcw', 'c', 'cpp',
  version: '1.51d',
  meson_version: '>=1.1.0',
  default_options: [
    'c_std=gnu11',
    'cpp_std=gnu++11',
    'buildtype=debugoptimized',
    'warning_level=2',
  ],
)

cc = meson.get_compiler('c')
if cc.get_argument_syntax() != 'gcc'
  error('The Meson port currently requires a GCC/Clang-compatible toolchain.')
endif

python = import('python').find_installation('python3')
source_list_script = meson.project_source_root() / 'scripts' / 'meson_source_list.py'

wolf_win32 = host_machine.system() == 'windows'
wolf_darwin = host_machine.system() == 'darwin'

wolf_arch = host_machine.cpu_family()
if wolf_arch == 'aarch64'
  wolf_arch = 'arm64'
endif

wolf_file_arch = wolf_arch
if wolf_win32
  if wolf_arch == 'x86_64'
    wolf_file_arch = 'x64'
  elif wolf_arch == 'x86'
    wolf_file_arch = 'x86'
  endif
elif wolf_arch == 'x86'
  wolf_file_arch = 'i386'
endif

wolf_install_bindir = '.'
wolf_install_gamedir = get_option('basegame')
if wolf_install_gamedir == ''
  wolf_install_gamedir = 'main'
endif

# Legacy Makefiles use `ifdef MINGW` to select Win32 source sets.
# For source-list extraction, treat any Windows target as that branch.
wolf_mingw = wolf_win32
wolf_has_windres = false
if wolf_win32
  windres_prog = find_program(
    ['windres', 'llvm-windres', 'x86_64-w64-mingw32-windres', 'i686-w64-mingw32-windres'],
    required: false,
  )
  wolf_has_windres = windres_prog.found()
endif

wolf_has_msvc_abi = false
if wolf_win32
  wolf_has_msvc_abi = cc.get_define('_MSC_VER') != ''
endif

wolf_have_vm_compiled = get_option('have_vm_compiled')
if wolf_have_vm_compiled and wolf_has_msvc_abi and wolf_arch == 'x86_64'
  warning('Disabling compiled VM JIT for MSVC-ABI x86_64 builds (qvmcall64 helper is unavailable in this toolchain path).')
  wolf_have_vm_compiled = false
endif

common_args = [
  '-DUSE_ICON',
  '-DARCH_STRING="' + wolf_file_arch + '"',
  '-DSDL_ENABLE_OLD_NAMES',
]

if not wolf_win32
  common_args += '-D_GNU_SOURCE'
endif

if get_option('default_basedir') != ''
  common_args += '-DDEFAULT_BASEDIR="' + get_option('default_basedir') + '"'
endif

if get_option('use_local_headers')
  warning('Option use_local_headers is deprecated and ignored. Dependency headers are resolved from wraps/subprojects.')
endif

if get_option('use_internal_codecs') or get_option('use_internal_zlib') or get_option('use_internal_jpeg') or get_option('use_internal_freetype')
  warning('Internal dependency source options are deprecated and ignored. Dependencies are resolved from wraps/subprojects.')
endif

if get_option('use_xdg')
  common_args += '-DUSE_XDG'
endif

if get_option('build_standalone')
  common_args += '-DSTANDALONE'
endif

if get_option('use_authorize_server')
  common_args += '-DUSE_AUTHORIZE_SERVER'
endif

if get_option('use_renderer_dlopen')
  common_args += '-DUSE_RENDERER_DLOPEN'
endif

if get_option('use_bloom')
  common_args += '-DUSE_BLOOM'
endif

if not wolf_have_vm_compiled
  common_args += '-DNO_VM_COMPILED'
endif

if wolf_has_msvc_abi
  common_args += [
    '-D_CRT_SECURE_NO_WARNINGS',
    '-D_CRT_NONSTDC_NO_WARNINGS',
  ]
endif

warning_args = cc.get_supported_arguments([
  '-fno-strict-aliasing',
  '-Wformat=2',
  '-Wformat-security',
  '-Wno-format-nonliteral',
  '-Wstrict-aliasing=2',
  '-Wmissing-format-attribute',
  '-Wdisabled-optimization',
  '-Wno-missing-field-initializers',
  '-Wno-unused-but-set-variable',
  '-Wno-deprecated-non-prototype',
  '-Wno-unused-parameter',
  '-Wno-cast-function-type-mismatch',
  '-Wno-deprecated-copy-with-user-provided-copy',
  '-Wno-shift-op-parentheses',
  '-Wno-bitwise-op-parentheses',
  '-Wno-logical-op-parentheses',
  '-Wno-logical-not-parentheses',
  '-Wno-implicit-const-int-float-conversion',
  '-Wno-absolute-value',
])

external_warning_silence_args = cc.get_supported_arguments([
  '-Wno-unused-but-set-parameter',
  '-Wno-null-pointer-arithmetic',
  '-Wno-null-pointer-subtraction',
  '-Wno-unused-parameter',
  '-Wno-missing-field-initializers',
  '-Wno-deprecated-non-prototype',
  '-Wno-shift-op-parentheses',
  '-Wno-implicit-const-int-float-conversion',
  '-Wno-inconsistent-dllimport',
  '-Wno-deprecated-copy-with-user-provided-copy',
])

external_warning_silence_link_args = []
if wolf_has_msvc_abi
  external_warning_silence_link_args = cc.get_supported_link_arguments([
    '-Wl,/ignore:4217',
    '-Wl,/ignore:4286',
  ])
endif

add_project_arguments(common_args + warning_args, language: ['c', 'cpp'])

if wolf_has_msvc_abi and get_option('build_client') and get_option('use_freetype') and external_warning_silence_link_args.length() > 0
  add_project_link_arguments(external_warning_silence_link_args, language: ['c', 'cpp'])
endif

fallback_static = ['default_library=static']
fallback_shared = ['default_library=shared']

threads_dep = dependency('threads')

wolf_libm_dep = disabler()
wolf_libdl_dep = disabler()
wolf_librt_dep = disabler()
wolf_ws2_dep = disabler()
wolf_winmm_dep = disabler()
wolf_psapi_dep = disabler()
wolf_gdi32_dep = disabler()
wolf_ole32_dep = disabler()

if wolf_win32
  wolf_ws2_dep = cc.find_library('ws2_32', required: true)
  wolf_winmm_dep = cc.find_library('winmm', required: true)
  wolf_psapi_dep = cc.find_library('psapi', required: true)
  wolf_gdi32_dep = cc.find_library('gdi32', required: true)
  wolf_ole32_dep = cc.find_library('ole32', required: true)
else
  wolf_libm_dep = cc.find_library('m', required: true)
  wolf_libdl_dep = cc.find_library('dl', required: false)
  wolf_librt_dep = cc.find_library('rt', required: false)
endif

zlib_dep = disabler()
need_zlib = get_option('build_client') or get_option('build_server')
if need_zlib
  zlib_dep = dependency('zlib',
    required: true,
    default_options: fallback_static + ['warning_level=0', 'werror=false'],
  )
endif

jpeg_dep = disabler()
need_jpeg = get_option('build_client')
if need_jpeg
  jpeg_dep = dependency('libjpeg',
    required: true,
    default_options: fallback_static + [
      'warning_level=0',
      'werror=false',
      'tests=disabled',
      'turbojpeg=disabled',
    ],
  )
endif

freetype_dep = disabler()
need_freetype = get_option('build_client') and get_option('use_freetype')
if need_freetype
  freetype_dep = dependency('freetype2',
    required: true,
    default_options: fallback_static + [
      'warning_level=0',
      'werror=false',
      'tests=disabled',
      'harfbuzz=disabled',
      'brotli=disabled',
      'png=disabled',
      'bzip2=disabled',
      'zlib=system',
    ],
  )
endif

need_opus = get_option('use_codec_opus') or get_option('use_voip')
need_ogg = need_opus or get_option('use_codec_vorbis')
need_opusfile = get_option('use_codec_opus')
need_vorbisfile = get_option('use_codec_vorbis')

ogg_dep = disabler()
if get_option('build_client') and need_ogg
  ogg_dep = dependency('ogg',
    required: true,
    default_options: fallback_static + ['warning_level=0', 'werror=false'],
  )
endif

vorbisfile_dep = disabler()
if get_option('build_client') and need_vorbisfile
  vorbisfile_dep = dependency('vorbisfile',
    required: true,
    default_options: fallback_static + ['warning_level=0', 'werror=false'],
  )
endif

opus_dep = disabler()
if (get_option('build_client') and need_opus) or (get_option('build_server') and get_option('use_voip'))
  opus_dep = dependency('opus',
    required: true,
    default_options: fallback_static + [
      'warning_level=0',
      'werror=false',
      'tests=disabled',
      'extra-programs=disabled',
      'docs=disabled',
    ],
  )
endif

opusfile_dep = disabler()
if get_option('build_client') and need_opusfile
  opusfile_dep = dependency('opusfile',
    required: true,
    default_options: fallback_static + ['warning_level=0', 'werror=false'],
  )
endif

sdl3_dep = disabler()
if get_option('build_client')
  sdl3_dep = dependency('sdl3',
    required: true,
    default_options: fallback_shared + ['warning_level=0', 'werror=false'],
  )

  if wolf_win32
    meson.add_install_script(
      python,
      meson.project_source_root() / 'scripts' / 'install_sdl_runtime.py',
    )
  endif
endif

openal_dep = disabler()
need_openal = get_option('build_client') and get_option('use_openal') and not get_option('use_openal_dlopen')
if need_openal
  openal_dep = dependency('openal',
    required: true,
    default_options: fallback_static + ['warning_level=0', 'werror=false'],
  )
endif

need_curl = (get_option('build_sp') and get_option('sp_curl')) or (get_option('build_mp') and get_option('mp_curl'))
curl_dep = disabler()
curl_headers_dep = disabler()
if get_option('build_client') and need_curl
  curl_resolved_dep = dependency('libcurl',
    required: true,
    default_options: fallback_static + [
      'warning_level=0',
      'werror=false',
      'tool=disabled',
      'tests=disabled',
      'unittests=disabled',
    ],
  )
  curl_headers_dep = curl_resolved_dep.partial_dependency(
    compile_args: true,
    includes: true,
  )
  if not get_option('use_curl_dlopen')
    curl_dep = curl_resolved_dep
  endif
endif

wolf_stringify = disabler()
if get_option('build_client') and get_option('build_renderer_rend2') and (get_option('build_sp') or get_option('build_mp'))
  wolf_stringify_args = []
  if wolf_has_msvc_abi
    wolf_stringify_args += [
      '-D_CRT_SECURE_NO_WARNINGS',
      '-D_CRT_NONSTDC_NO_WARNINGS',
    ]
  endif
  wolf_stringify = executable('wolf_stringify', 'SP/code/tools/stringify.c',
    c_args: wolf_stringify_args,
    native: true,
  )
endif

if get_option('build_sp')
  subdir('SP')
endif

if get_option('build_mp')
  subdir('MP')
endif

summary({
  'build_sp': get_option('build_sp'),
  'build_mp': get_option('build_mp'),
  'build_client': get_option('build_client'),
  'build_server': get_option('build_server'),
  'build_game_so': get_option('build_game_so'),
  'build_renderer_rend2': get_option('build_renderer_rend2'),
  'use_renderer_dlopen': get_option('use_renderer_dlopen'),
  'use_openal': get_option('use_openal'),
  'use_openal_dlopen': get_option('use_openal_dlopen'),
  'sp_curl': get_option('sp_curl'),
  'mp_curl': get_option('mp_curl'),
  'use_curl_dlopen': get_option('use_curl_dlopen'),
  'use_codec_vorbis': get_option('use_codec_vorbis'),
  'use_codec_opus': get_option('use_codec_opus'),
  'use_voip': get_option('use_voip'),
  'use_mumble': get_option('use_mumble'),
  'use_freetype': get_option('use_freetype'),
  'use_internal_codecs': get_option('use_internal_codecs'),
  'use_internal_zlib': get_option('use_internal_zlib'),
  'use_internal_jpeg': get_option('use_internal_jpeg'),
  'use_internal_freetype': get_option('use_internal_freetype'),
  'have_vm_compiled': wolf_have_vm_compiled,
  'arch': wolf_arch,
  'file_arch': wolf_file_arch,
}, section: 'iortcw Meson')
