fs = import('fs')

sp_inc_dirs = [
  'code',
]

sp_inc = include_directories(sp_inc_dirs)

sp_source_groups = {}
foreach group_name : ['Q3OBJ', 'Q3ROBJ', 'Q3R2OBJ', 'Q3R2STRINGOBJ', 'JPGOBJ', 'FTOBJ', 'Q3DOBJ', 'Q3CGOBJ', 'Q3GOBJ', 'Q3UIOBJ']
  cmd = [
    python,
    source_list_script,
    '--game', 'SP',
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
    '--have-vm-compiled', wolf_have_vm_compiled ? '1' : '0',
  ]
  result = run_command(cmd, check: true).stdout().strip()
  group_sources = []
  if result != ''
    foreach src : result.split('\n')
      group_sources += meson.project_source_root() / src
    endforeach
  endif
  sp_source_groups += {group_name: group_sources}
endforeach

sp_q3obj = sp_source_groups['Q3OBJ']
sp_q3robj = sp_source_groups['Q3ROBJ']
sp_q3r2obj = sp_source_groups['Q3R2OBJ']
sp_q3r2glsl = sp_source_groups['Q3R2STRINGOBJ']
sp_jpgobj = sp_source_groups['JPGOBJ']
sp_ftobj = sp_source_groups['FTOBJ']
sp_q3dobj = sp_source_groups['Q3DOBJ']
sp_q3cgobj = sp_source_groups['Q3CGOBJ']
sp_q3gobj = sp_source_groups['Q3GOBJ']
sp_q3uiobj = sp_source_groups['Q3UIOBJ']

if wolf_win32 and not wolf_has_windres
  sp_q3obj_filtered = []
  foreach src : sp_q3obj
    if not src.endswith('.rc')
      sp_q3obj_filtered += src
    endif
  endforeach
  sp_q3obj = sp_q3obj_filtered

  sp_q3dobj_filtered = []
  foreach src : sp_q3dobj
    if not src.endswith('.rc')
      sp_q3dobj_filtered += src
    endif
  endforeach
  sp_q3dobj = sp_q3dobj_filtered
endif

sp_rend2_glsl_c = []
if get_option('build_renderer_rend2') and get_option('build_client')
  foreach glsl_file : sp_q3r2glsl
    glsl_name = fs.name(glsl_file)
    output_c = glsl_name.replace('.glsl', '.c')
    sp_rend2_glsl_c += custom_target(
      'sp_glsl_' + output_c,
      input: glsl_file,
      output: output_c,
      command: [wolf_stringify, '@INPUT@', '@OUTPUT@'],
    )
  endforeach
endif

sp_client_deps = [threads_dep]
sp_renderer_deps = [threads_dep]
sp_server_deps = [threads_dep]
sp_game_deps = []

if not wolf_win32
  sp_client_deps += wolf_libm_dep
  sp_renderer_deps += wolf_libm_dep
  sp_server_deps += wolf_libm_dep
  sp_game_deps += wolf_libm_dep
  if wolf_libdl_dep.found()
    sp_client_deps += wolf_libdl_dep
    sp_renderer_deps += wolf_libdl_dep
    sp_server_deps += wolf_libdl_dep
  endif
  if get_option('use_mumble') and wolf_librt_dep.found()
    sp_client_deps += wolf_librt_dep
  endif
else
  sp_client_deps += [wolf_ws2_dep, wolf_winmm_dep, wolf_psapi_dep, wolf_gdi32_dep, wolf_ole32_dep]
  sp_renderer_deps += [wolf_gdi32_dep, wolf_ole32_dep]
  sp_server_deps += [wolf_ws2_dep, wolf_winmm_dep, wolf_psapi_dep]
endif

sp_client_deps += zlib_dep
sp_server_deps += zlib_dep

sp_renderer_sources = sp_q3robj + sp_jpgobj + sp_ftobj
sp_renderer_rend2_sources = sp_q3r2obj + sp_rend2_glsl_c + sp_jpgobj + sp_ftobj

sp_renderer_deps += jpeg_dep

if get_option('use_freetype')
  sp_renderer_deps += freetype_dep
endif

if get_option('build_client')
  sp_client_deps += sdl3_dep
  sp_renderer_deps += sdl3_dep
endif

if get_option('use_codec_vorbis')
  sp_client_deps += [ogg_dep, vorbisfile_dep]
endif

if get_option('use_codec_opus')
  sp_client_deps += [ogg_dep, opus_dep, opusfile_dep]
endif

if get_option('use_voip')
  sp_client_deps += opus_dep
  sp_server_deps += opus_dep
endif

if get_option('use_openal') and not get_option('use_openal_dlopen')
  sp_client_deps += openal_dep
endif

if get_option('sp_curl')
  if curl_headers_dep.found()
    sp_client_deps += curl_headers_dep
  endif
  if not get_option('use_curl_dlopen')
    sp_client_deps += curl_dep
  endif
endif

if get_option('sp_curl')
  sp_client_args = ['-DUSE_CURL']
  if get_option('use_curl_dlopen')
    sp_client_args += '-DUSE_CURL_DLOPEN'
  endif
else
  sp_client_args = []
endif

if get_option('use_openal')
  sp_client_args += '-DUSE_OPENAL'
  if get_option('use_openal_dlopen')
    sp_client_args += '-DUSE_OPENAL_DLOPEN'
  endif
endif

if get_option('use_voip')
  sp_client_args += '-DUSE_VOIP'
endif

if get_option('use_codec_opus')
  sp_client_args += '-DUSE_CODEC_OPUS'
endif

if get_option('use_codec_vorbis')
  sp_client_args += '-DUSE_CODEC_VORBIS'
endif

if get_option('use_mumble')
  sp_client_args += '-DUSE_MUMBLE'
endif

if get_option('use_freetype')
  sp_client_args += '-DBUILD_FREETYPE'
endif

sp_server_args = []
if get_option('use_voip')
  sp_server_args += '-DUSE_VOIP'
endif

sp_common_target_args = [
  '-DPRODUCT_VERSION="' + get_option('version_sp') + '"',
]

sp_client_bin = wolf_win32 ? 'ioWolfSP' : 'iowolfsp'
sp_server_bin = wolf_win32 ? 'ioWolfSPDED' : 'iowolfspded'

if get_option('build_client')
  if get_option('use_renderer_dlopen')
    executable(sp_client_bin,
      sp_q3obj,
      c_args: sp_common_target_args + sp_client_args + ['-DBOTLIB'],
      include_directories: sp_inc,
      dependencies: sp_client_deps,
      install: true,
      install_dir: wolf_install_bindir,
    )

    shared_library('renderer_sp_opengl1_' + wolf_file_arch,
      sp_renderer_sources,
      name_prefix: '',
      c_args: sp_common_target_args + sp_client_args,
      include_directories: sp_inc,
      dependencies: sp_renderer_deps,
      gnu_symbol_visibility: 'hidden',
      install: true,
      install_dir: wolf_install_bindir,
    )

    if get_option('build_renderer_rend2')
      shared_library('renderer_sp_rend2_' + wolf_file_arch,
        sp_renderer_rend2_sources,
        name_prefix: '',
        c_args: sp_common_target_args + sp_client_args,
        include_directories: sp_inc,
        dependencies: sp_renderer_deps,
        gnu_symbol_visibility: 'hidden',
        install: true,
        install_dir: wolf_install_bindir,
      )
    endif
  else
    executable(sp_client_bin,
      sp_q3obj + sp_renderer_sources,
      c_args: sp_common_target_args + sp_client_args + ['-DBOTLIB'],
      include_directories: sp_inc,
      dependencies: sp_client_deps + sp_renderer_deps,
      install: true,
      install_dir: wolf_install_bindir,
    )

    if get_option('build_renderer_rend2')
      executable(sp_client_bin + '_rend2',
        sp_q3obj + sp_renderer_rend2_sources,
        c_args: sp_common_target_args + sp_client_args + ['-DBOTLIB'],
        include_directories: sp_inc,
        dependencies: sp_client_deps + sp_renderer_deps,
        install: true,
        install_dir: wolf_install_bindir,
      )
    endif
  endif
endif

if get_option('build_server')
  executable(sp_server_bin,
    sp_q3dobj,
    c_args: sp_common_target_args + sp_server_args + ['-DDEDICATED', '-DBOTLIB'],
    include_directories: sp_inc,
    dependencies: sp_server_deps,
    install: true,
    install_dir: wolf_install_bindir,
  )
endif

if get_option('build_game_so')
  if wolf_win32
    sp_cgame_name = 'cgame_sp_' + wolf_file_arch
    sp_qagame_name = 'qagame_sp_' + wolf_file_arch
    sp_ui_name = 'ui_sp_' + wolf_file_arch
  else
    sp_cgame_name = 'cgame.sp.' + wolf_file_arch
    sp_qagame_name = 'qagame.sp.' + wolf_file_arch
    sp_ui_name = 'ui.sp.' + wolf_file_arch
  endif

  shared_library(sp_cgame_name,
    sp_q3cgobj,
    name_prefix: '',
    c_args: sp_common_target_args + ['-DCGAMEDLL', '-DCGAME'],
    include_directories: sp_inc,
    dependencies: sp_game_deps,
    gnu_symbol_visibility: 'hidden',
    install: true,
    install_dir: wolf_install_gamedir,
  )

  shared_library(sp_qagame_name,
    sp_q3gobj,
    name_prefix: '',
    c_args: sp_common_target_args + ['-DGAMEDLL', '-DQAGAME'],
    include_directories: sp_inc,
    dependencies: sp_game_deps,
    gnu_symbol_visibility: 'hidden',
    install: true,
    install_dir: wolf_install_gamedir,
  )

  shared_library(sp_ui_name,
    sp_q3uiobj,
    name_prefix: '',
    c_args: sp_common_target_args + ['-DUI'],
    include_directories: sp_inc,
    dependencies: sp_game_deps,
    gnu_symbol_visibility: 'hidden',
    install: true,
    install_dir: wolf_install_gamedir,
  )
endif
