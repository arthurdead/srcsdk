project('recastnavigation_game', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

game_dir = get_option('game_dir')
build_libs = get_option('build_libs')

cmake = import('cmake')

recastnavigation_opts = cmake.subproject_options()
recastnavigation_opts.add_cmake_defines({
	'CMAKE_POSITION_INDEPENDENT_CODE': true,
	'BUILD_SHARED_LIBS': true,
	'RECASTNAVIGATION_DEMO': false,
	'RECASTNAVIGATION_TESTS': false,
	'RECASTNAVIGATION_EXAMPLES': false,
	'RECASTNAVIGATION_DT_POLYREF64': false,
	'RECASTNAVIGATION_DT_VIRTUAL_QUERYFILTER': true,
	'CMAKE_INSTALL_PREFIX': game_dir,
	'CMAKE_INSTALL_FULL_LIBDIR': join_paths(game_dir,'bin'),
	'CMAKE_INSTALL_LIBDIR': 'bin',
	'CMAKE_INSTALL_FULL_BINDIR': join_paths(game_dir,'bin'),
	'CMAKE_INSTALL_BINDIR': 'bin'
})

if not build_libs
	recastnavigation_opts.set_install(false)
	recastnavigation_opts.set_override_option('build_by_default','false')
endif

recastnavigation = cmake.subproject('recastnavigation', options: recastnavigation_opts)

recastnavigation_game_dep = declare_dependency(
	dependencies: [
		recastnavigation.dependency('Recast'),
		recastnavigation.dependency('DetourTileCache'),
		recastnavigation.dependency('DetourCrowd'),
		recastnavigation.dependency('Detour'),
		recastnavigation.dependency('DebugUtils'),
	]
)