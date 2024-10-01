project('rmlui_game', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

game_dir = get_option('game_dir')
build_client = get_option('build_client')

cmake = import('cmake')

rmlui_opts = cmake.subproject_options()
rmlui_opts.add_cmake_defines({
	'CMAKE_POSITION_INDEPENDENT_CODE': true,
	'BUILD_SHARED_LIBS': true,
	'BUILD_TESTING': false,
	'RMLUI_SAMPLES': false,
	'RMLUI_FONT_ENGINE': 'freetype',
	'RMLUI_LUA_BINDINGS': false,
	'RMLUI_LOTTIE_PLUGIN': false,
	'RMLUI_SVG_PLUGIN': false,
	'RMLUI_HARFBUZZ_SAMPLE': false,
	'RMLUI_THIRDPARTY_CONTAINERS': true,
	'RMLUI_MATRIX_ROW_MAJOR': false,
	'RMLUI_CUSTOM_RTTI': false,
	'RMLUI_PRECOMPILED_HEADERS': true,
	'RMLUI_COMPILER_OPTIONS': true,
	'RMLUI_WARNINGS_AS_ERRORS': false,
	'RMLUI_TRACY_PROFILING': false,
	'CMAKE_INSTALL_PREFIX': game_dir,
	'CMAKE_INSTALL_FULL_LIBDIR': join_paths(game_dir,'bin'),
	'CMAKE_INSTALL_LIBDIR': 'bin',
	'CMAKE_INSTALL_FULL_BINDIR': join_paths(game_dir,'bin'),
	'CMAKE_INSTALL_BINDIR': 'bin'
})

rmlui_opts.set_install(false)
rmlui_opts.set_override_option('build_by_default','false')

rmlui_base = cmake.subproject('RmlUi', options: rmlui_opts)

rmlui_core = shared_library('rmlui_core',
	cpp_args : ['-DRMLUI_CORE_EXPORTS', '-Drmlui_core_EXPORTS'],
	include_directories: rmlui_base.get_variable('rmlui_core_inc'),
	sources: rmlui_base.get_variable('rmlui_core_src'),
	dependencies: [dependency('freetype2')]
)

rmlui_core_dep = declare_dependency(
	include_directories : rmlui_base.get_variable('rmlui_core_inc'),
	link_with: rmlui_core,
)

rmlui_game_dep = declare_dependency(
	dependencies: [
		rmlui_core_dep,
	]
)