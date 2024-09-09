project('webm_game', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

game_dir = get_option('game_dir')
build_client = get_option('build_client')

cmake = import('cmake')

webm_opts = cmake.subproject_options()
webm_opts.add_cmake_defines({
	'CMAKE_POSITION_INDEPENDENT_CODE': true,
	'BUILD_SHARED_LIBS': true,
	'ENABLE_WEBMTS': true,
	'ENABLE_WEBMINFO': false,
	'ENABLE_TESTS': false,
	'ENABLE_IWYU': false,
	'ENABLE_WERROR': false,
	'ENABLE_WEBM_PARSER': true,
	'ENABLE_SAMPLE_PROGRAMS': false,
	'CMAKE_INSTALL_PREFIX': game_dir,
	'CMAKE_INSTALL_FULL_LIBDIR': join_paths(game_dir,'bin'),
	'CMAKE_INSTALL_LIBDIR': 'bin',
	'CMAKE_INSTALL_FULL_BINDIR': join_paths(game_dir,'bin'),
	'CMAKE_INSTALL_BINDIR': 'bin'
})

if not build_client
	webm_opts.set_install(false)
	webm_opts.set_override_option('build_by_default','false')
endif

webm = cmake.subproject('webm', options: webm_opts)

webm_game_dep = declare_dependency(
	dependencies: [
		webm.dependency('webm'),
	]
)