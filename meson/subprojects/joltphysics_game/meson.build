project('joltphysics_game', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

game_dir = get_option('game_dir')
build_libs = get_option('build_libs')

cmake = import('cmake')

joltphysics_opts = cmake.subproject_options()
joltphysics_opts.add_cmake_defines({
	'CMAKE_POSITION_INDEPENDENT_CODE': true,
	'BUILD_SHARED_LIBS': true,
	'CMAKE_INSTALL_PREFIX': game_dir,
	'CMAKE_INSTALL_FULL_LIBDIR': join_paths(game_dir,'bin'),
	'CMAKE_INSTALL_LIBDIR': 'bin',
	'CMAKE_INSTALL_FULL_BINDIR': join_paths(game_dir,'bin'),
	'CMAKE_INSTALL_BINDIR': 'bin',
	'USE_ASSERTS': false,
	'DOUBLE_PRECISION': false,
	'GENERATE_DEBUG_SYMBOLS': false,
	'INTERPROCEDURAL_OPTIMIZATION': true,
	'FLOATING_POINT_EXCEPTIONS_ENABLED': false,
	'CPP_EXCEPTIONS_ENABLED': false,
	'CPP_RTTI_ENABLED': true,
	'OBJECT_LAYER_BITS': 16,
	'USE_SSE4_1': true,
	'USE_SSE4_2': true,
	'USE_AVX': false,
	'USE_AVX2': false,
	'USE_AVX512': false,
	'USE_LZCNT': false,
	'USE_TZCNT': false,
	'USE_F16C': false,
	'USE_FMADD': false,
	'ENABLE_ALL_WARNINGS': false,
	'TRACK_BROADPHASE_STATS': false,
	'TRACK_NARROWPHASE_STATS': false,
	'DEBUG_RENDERER_IN_DEBUG_AND_RELEASE': false,
	'DEBUG_RENDERER_IN_DISTRIBUTION': false,
	'PROFILER_IN_DEBUG_AND_RELEASE': false,
	'PROFILER_IN_DISTRIBUTION': false,
	'DISABLE_CUSTOM_ALLOCATOR': true,
	'USE_STD_VECTOR': false,
	'ENABLE_OBJECT_STREAM': true,
	'TARGET_UNIT_TESTS': false,
	'TARGET_HELLO_WORLD': false,
	'TARGET_PERFORMANCE_TEST': false,
	'TARGET_SAMPLES': false,
	'TARGET_VIEWER': false,
})

if not build_libs
	joltphysics_opts.add_cmake_defines({
		'ENABLE_INSTALL': false
	})

	joltphysics_opts.set_install(false)
	joltphysics_opts.set_override_option('build_by_default','false')
else
	joltphysics_opts.add_cmake_defines({
		'ENABLE_INSTALL': true
	})

	joltphysics_opts.set_override_option('install_dir', join_paths(game_dir,'bin'))
endif

joltphysics = cmake.subproject('JoltPhysics', options: joltphysics_opts)

joltphysics_game_dep = declare_dependency(
	dependencies: [
		joltphysics.dependency('Jolt'),
	]
)