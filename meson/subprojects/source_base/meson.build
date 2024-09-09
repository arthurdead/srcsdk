project('source_base','cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
src_engine_dir = get_option('src_engine_dir')
game_dir = get_option('game_dir')

common_dir = join_paths(src_root_dir,'common')

source_debug_dep = declare_dependency(
	compile_args: [
		'-DFRAME_POINTER_OMISSION_DISABLED',
		'-DDEV_BUILD',
		'-DSTAGING_ONLY',
		'-DDEBUG',
		'-D_DEBUG'
	]
)

source_release_dep = declare_dependency(
	compile_args: [
		'-DNDEBUG',
		'-D_NDEBUG'
	]
)

source_dep = declare_dependency(
	compile_args: [
		'-DRAD_TELEMETRY_DISABLED',
		'-DSDK2013CE',
		'-DVERSION_SAFE_STEAM_API_INTERFACES',
		'-DMOVE_CONSTRUCTOR_SUPPORT',
		'-DVALVE_RVALUE_REFS',
		'-DVALVE_INITIALIZER_LIST_SUPPORT',
		'-Wno-unknown-pragmas',
		'-Wno-unused-local-typedefs',
		'-Wno-template-id-cdtor',
		'-Wno-unused-value',
		'-Wno-sign-compare',
		'-Wno-reorder',
		'-Wno-class-memaccess',
		'-Wno-register',
		'-Wno-misleading-indentation',
		'-Wno-nonnull-compare',
		'-Wno-overloaded-virtual',
		'-Wno-ignored-attributes',
		'-Wno-unused-but-set-variable',
		'-Wno-write-strings',
		'-Wno-unused-function',
		'-Wno-unused-variable',
		'-Wno-c++20-compat',
		'-Wno-delete-non-virtual-dtor',
		'-Wno-narrowing',
		'-Wno-mismatched-new-delete',
		'-Wno-nonnull',
		'-Wno-address',
		'-Wno-invalid-offsetof',
		'-Wno-conversion-null',
		'-Wno-switch',
		'-Wno-uninitialized',
		'-Wno-logical-not-parentheses',
		'-Wno-int-in-bool-context',
		'-Wno-sequence-point'
	],
	include_directories: include_directories(
		join_paths(src_root_dir,'public'),
		join_paths(src_root_dir,'common'),
		join_paths(src_root_dir,'utils','common')
	),
	link_args: [
		'-L'+join_paths(game_dir,'bin'),
		'-L'+join_paths(src_engine_dir,'bin'),
		'-L'+join_paths(src_root_dir,'lib','public','linux32')
	],
	dependencies: [
		source_debug_dep
	]
)

source_slop_dep = declare_dependency(
	compile_args: [
		#'-DVECTOR',
		#'-DPROTECTED_THINGS_ENABLE',
		#'-Dsprintf=use_Q_snprintf_instead_of_sprintf',
		#'-Dstrncpy=use_Q_strncpy_instead',
		#'-D_snprintf=use_Q_snprintf_instead'
	]
)

source_static_lib_dep_base = declare_dependency(
	dependencies: [
		source_dep
	],
)

source_shared_lib_dep_base = declare_dependency(
	dependencies: [
		source_dep
	],
	sources: files(
		join_paths(common_dir,'debug_dll_check.cpp')
	)
)

source_linux_dep = declare_dependency(
	compile_args: [
		'-DGNUC',
		'-DCOMPILER_GCC',
		'-DLINUX',
		'-D_LINUX',
		'-DPOSIX',
		'-D_POSIX',
		'-D_DLL_EXT=.so',
		'-DUSE_SDL',
		'-DDX_TO_GL_ABSTRACTION',
		'-DGL_GLEXT_PROTOTYPES',
		'-DBINK_VIDEO',
		'-DNO_MALLOC_OVERRIDE',
		'-DNO_HOOK_MALLOC'
	]
)

source_windows_dep = declare_dependency(
	compile_args: [
		'-DGNUC',
		'-DCOMPILER_GCC',
		'-DWIN32',
		'-D_WIN32',
		'-D_DLL_EXT=.dll',
		'-DBINK_VIDEO',
	]
)

target_system = target_machine.system()
if target_system == 'linux' or target_system == 'gnu'
	os_dep = source_linux_dep
elif target_system == 'windows'
	os_dep = source_windows_dep
else
	error('unsupported system')
endif

source_static_lib_dep = declare_dependency(
	dependencies: [
		source_static_lib_dep_base,
		os_dep
	],
)

source_shared_lib_dep = declare_dependency(
	dependencies: [
		source_shared_lib_dep_base,
		os_dep
	],
)
