fs = import('fs')

mp_inc_dirs = [
  'code',
]

mp_inc = include_directories(mp_inc_dirs)

mp_source_groups = {}
foreach group_name : ['Q3OBJ', 'Q3ROBJ', 'Q3R2OBJ', 'Q3R2STRINGOBJ', 'JPGOBJ', 'FTOBJ', 'Q3DOBJ', 'Q3CGOBJ', 'Q3GOBJ', 'Q3UIOBJ']
  cmd = [
    python,
    source_list_script,
    '--game', 'MP',
    '--group', group_name,
    '--arch', wolf_arch,
    '--platform', host_machine.system(),
    '--mingw', wolf_mingw ? '1' : '0',
    '--basegame', get_option('basegame'),
    '--use-bloom', get_option('use_bloom') ? '1' : '0',
    '--use-renderer-dlopen', get_option('use_renderer_dlopen') ? '1' : '0',
    '--use-internal-jpeg', '0',
    '--use-freetype', get_option('use_freetype') ? '1' : '0',
    '--use-internal-freetype', '0',
    '--use-internal-opus', '0',
    '--use-internal-ogg', '0',
    '--use-internal-vorbis', '0',
    '--use-internal-zlib', '0',
    '--use-codec-vorbis', get_option('use_codec_vorbis') ? '1' : '0',
    '--use-codec-opus', get_option('use_codec_opus') ? '1' : '0',
    '--use-voip', get_option('use_voip') ? '1' : '0',
    '--use-mumble', get_option('use_mumble') ? '1' : '0',
    '--use-antiwallhack', get_option('use_antiwallhack') ? '1' : '0',
    '--have-vm-compiled', wolf_have_vm_compiled ? '1' : '0',
  ]
  result = run_command(cmd, check: true).stdout().strip()
  group_sources = []
  if result != ''
    foreach src : result.split('\n')
      group_sources += meson.project_source_root() / src
    endforeach
  endif
  mp_source_groups += {group_name: group_sources}
endforeach

mp_q3obj = mp_source_groups['Q3OBJ']
mp_q3robj = mp_source_groups['Q3ROBJ']
mp_q3r2obj = mp_source_groups['Q3R2OBJ']
mp_q3r2glsl = mp_source_groups['Q3R2STRINGOBJ']
mp_jpgobj = mp_source_groups['JPGOBJ']
mp_ftobj = mp_source_groups['FTOBJ']
mp_q3dobj = mp_source_groups['Q3DOBJ']
mp_q3cgobj = mp_source_groups['Q3CGOBJ']
mp_q3gobj = mp_source_groups['Q3GOBJ']
mp_q3uiobj = mp_source_groups['Q3UIOBJ']

if wolf_win32 and not wolf_has_windres
  mp_q3obj_filtered = []
  foreach src : mp_q3obj
    if not src.endswith('.rc')
      mp_q3obj_filtered += src
    endif
  endforeach
  mp_q3obj = mp_q3obj_filtered

  mp_q3dobj_filtered = []
  foreach src : mp_q3dobj
    if not src.endswith('.rc')
      mp_q3dobj_filtered += src
    endif
  endforeach
  mp_q3dobj = mp_q3dobj_filtered
endif

mp_rend2_glsl_c = []
if get_option('build_renderer_rend2') and get_option('build_client')
  foreach glsl_file : mp_q3r2glsl
    glsl_name = fs.name(glsl_file)
    output_c = glsl_name.replace('.glsl', '.c')
    mp_rend2_glsl_c += custom_target(
      'mp_glsl_' + output_c,
      input: glsl_file,
      output: output_c,
      command: [wolf_stringify, '@INPUT@', '@OUTPUT@'],
    )
  endforeach
endif

mp_client_deps = [threads_dep]
mp_renderer_deps = [threads_dep]
mp_server_deps = [threads_dep]
mp_game_deps = []

if not wolf_win32
  mp_client_deps += wolf_libm_dep
  mp_renderer_deps += wolf_libm_dep
  mp_server_deps += wolf_libm_dep
  mp_game_deps += wolf_libm_dep
  if wolf_libdl_dep.found()
    mp_client_deps += wolf_libdl_dep
    mp_renderer_deps += wolf_libdl_dep
    mp_server_deps += wolf_libdl_dep
  endif
  if get_option('use_mumble') and wolf_librt_dep.found()
    mp_client_deps += wolf_librt_dep
  endif
else
  mp_client_deps += [wolf_ws2_dep, wolf_winmm_dep, wolf_psapi_dep, wolf_gdi32_dep, wolf_ole32_dep]
  mp_renderer_deps += [wolf_gdi32_dep, wolf_ole32_dep]
  mp_server_deps += [wolf_ws2_dep, wolf_winmm_dep, wolf_psapi_dep]
endif

mp_client_deps += zlib_dep
mp_server_deps += zlib_dep

mp_renderer_sources = mp_q3robj + mp_jpgobj + mp_ftobj
mp_renderer_rend2_sources = mp_q3r2obj + mp_rend2_glsl_c + mp_jpgobj + mp_ftobj

mp_renderer_deps += jpeg_dep

if get_option('use_freetype')
  mp_renderer_deps += freetype_dep
endif

if get_option('build_client')
  mp_client_deps += sdl3_dep
  mp_renderer_deps += sdl3_dep
endif

if get_option('use_codec_vorbis')
  mp_client_deps += [ogg_dep, vorbisfile_dep]
endif

if get_option('use_codec_opus')
  mp_client_deps += [ogg_dep, opus_dep, opusfile_dep]
endif

if get_option('use_voip')
  mp_client_deps += opus_dep
  mp_server_deps += opus_dep
endif

if get_option('use_openal') and not get_option('use_openal_dlopen')
  mp_client_deps += openal_dep
endif

if get_option('mp_curl')
  if curl_headers_dep.found()
    mp_client_deps += curl_headers_dep
  endif
  if not get_option('use_curl_dlopen')
    mp_client_deps += curl_dep
  endif
endif

if get_option('mp_curl')
  mp_client_args = ['-DUSE_CURL']
  if get_option('use_curl_dlopen')
    mp_client_args += '-DUSE_CURL_DLOPEN'
  endif
else
  mp_client_args = []
endif

if get_option('use_openal')
  mp_client_args += '-DUSE_OPENAL'
  if get_option('use_openal_dlopen')
    mp_client_args += '-DUSE_OPENAL_DLOPEN'
  endif
endif

if get_option('use_voip')
  mp_client_args += '-DUSE_VOIP'
endif

if get_option('use_codec_opus')
  mp_client_args += '-DUSE_CODEC_OPUS'
endif

if get_option('use_codec_vorbis')
  mp_client_args += '-DUSE_CODEC_VORBIS'
endif

if get_option('use_mumble')
  mp_client_args += '-DUSE_MUMBLE'
endif

if get_option('use_freetype')
  mp_client_args += '-DBUILD_FREETYPE'
endif

mp_server_args = []
if get_option('use_voip')
  mp_server_args += '-DUSE_VOIP'
endif
if get_option('use_antiwallhack')
  mp_server_args += '-DANTIWALLHACK'
endif

mp_common_target_args = [
  '-DPRODUCT_VERSION="' + get_option('version_mp') + '"',
]

mp_client_bin = wolf_win32 ? 'ioWolfMP' : 'iowolfmp'
mp_server_bin = wolf_win32 ? 'ioWolfDED' : 'iowolfded'

if get_option('build_client')
  if get_option('use_renderer_dlopen')
    executable(mp_client_bin,
      mp_q3obj,
      c_args: mp_common_target_args + mp_client_args + ['-DBOTLIB'],
      include_directories: mp_inc,
      dependencies: mp_client_deps,
      install: true,
      install_dir: wolf_install_bindir,
    )

    shared_library('renderer_mp_opengl1_' + wolf_file_arch,
      mp_renderer_sources,
      name_prefix: '',
      c_args: mp_common_target_args + mp_client_args,
      include_directories: mp_inc,
      dependencies: mp_renderer_deps,
      gnu_symbol_visibility: 'hidden',
      install: true,
      install_dir: wolf_install_bindir,
    )

    if get_option('build_renderer_rend2')
      shared_library('renderer_mp_rend2_' + wolf_file_arch,
        mp_renderer_rend2_sources,
        name_prefix: '',
        c_args: mp_common_target_args + mp_client_args,
        include_directories: mp_inc,
        dependencies: mp_renderer_deps,
        gnu_symbol_visibility: 'hidden',
        install: true,
        install_dir: wolf_install_bindir,
      )
    endif
  else
    executable(mp_client_bin,
      mp_q3obj + mp_renderer_sources,
      c_args: mp_common_target_args + mp_client_args + ['-DBOTLIB'],
      include_directories: mp_inc,
      dependencies: mp_client_deps + mp_renderer_deps,
      install: true,
      install_dir: wolf_install_bindir,
    )

    if get_option('build_renderer_rend2')
      executable(mp_client_bin + '_rend2',
        mp_q3obj + mp_renderer_rend2_sources,
        c_args: mp_common_target_args + mp_client_args + ['-DBOTLIB'],
        include_directories: mp_inc,
        dependencies: mp_client_deps + mp_renderer_deps,
        install: true,
        install_dir: wolf_install_bindir,
      )
    endif
  endif
endif

if get_option('build_server')
  executable(mp_server_bin,
    mp_q3dobj,
    c_args: mp_common_target_args + mp_server_args + ['-DDEDICATED', '-DBOTLIB'],
    include_directories: mp_inc,
    dependencies: mp_server_deps,
    install: true,
    install_dir: wolf_install_bindir,
  )
endif

if get_option('build_game_so')
  if wolf_win32
    mp_cgame_name = 'cgame_mp_' + wolf_file_arch
    mp_qagame_name = 'qagame_mp_' + wolf_file_arch
    mp_ui_name = 'ui_mp_' + wolf_file_arch
  else
    mp_cgame_name = 'cgame.mp.' + wolf_file_arch
    mp_qagame_name = 'qagame.mp.' + wolf_file_arch
    mp_ui_name = 'ui.mp.' + wolf_file_arch
  endif

  shared_library(mp_cgame_name,
    mp_q3cgobj,
    name_prefix: '',
    c_args: mp_common_target_args + ['-DCGAMEDLL', '-DCGAME'],
    include_directories: mp_inc,
    dependencies: mp_game_deps,
    gnu_symbol_visibility: 'hidden',
    install: true,
    install_dir: wolf_install_gamedir,
  )

  shared_library(mp_qagame_name,
    mp_q3gobj,
    name_prefix: '',
    c_args: mp_common_target_args + ['-DGAMEDLL', '-DQAGAME'],
    include_directories: mp_inc,
    dependencies: mp_game_deps,
    gnu_symbol_visibility: 'hidden',
    install: true,
    install_dir: wolf_install_gamedir,
  )

  shared_library(mp_ui_name,
    mp_q3uiobj,
    name_prefix: '',
    c_args: mp_common_target_args + ['-DUI'],
    include_directories: mp_inc,
    dependencies: mp_game_deps,
    gnu_symbol_visibility: 'hidden',
    install: true,
    install_dir: wolf_install_gamedir,
  )
endif
