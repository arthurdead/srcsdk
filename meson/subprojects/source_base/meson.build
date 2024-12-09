project('source_base','cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
funny_src_root_dir = join_paths(src_root_dir,'..','src_funny')

src_engine_dir = get_option('src_engine_dir')
src_dedicated_engine_dir = get_option('src_dedicated_engine_dir')
game_dir = get_option('game_dir')
src_engine_target = get_option('src_engine_target')

common_dir = join_paths(src_root_dir,'common')
public_dir = join_paths(src_root_dir,'public')

source_debug_dep = declare_dependency(
	compile_args: [
		'-DFRAME_POINTER_OMISSION_DISABLED',
		'-DDEV_BUILD',
		'-DSTAGING_ONLY',
		'-DDEBUG',
		'-D_DEBUG',
		'-DVPROF_ENABLED'
	]
)

source_release_dep = declare_dependency(
	compile_args: [
		'-DNDEBUG',
		'-D_NDEBUG'
	]
)

target_system = target_machine.system()
if meson.get_compiler('cpp').has_define('__WINE__')
	lib_folder = join_paths(src_root_dir,'lib','public','linux32_wine',src_engine_target)
	funny_lib_folder = join_paths(funny_src_root_dir,'lib','public','linux32_wine',src_engine_target)
	lib_folders = [
		'-L'+lib_folder,
		'-L'+join_paths(src_root_dir,'lib','public','linux32',src_engine_target),
		'-L'+join_paths(src_root_dir,'lib','public','linux32'),
	]
	if get_option('funny')
		lib_folders += [
			'-L'+funny_lib_folder,
			'-L'+join_paths(funny_src_root_dir,'lib','public','linux32',src_engine_target),
			'-L'+join_paths(funny_src_root_dir,'lib','public','linux32')
		]
	endif
elif target_system == 'linux' or target_system == 'gnu'
	lib_folder = join_paths(src_root_dir,'lib','public','linux32',src_engine_target)
	funny_lib_folder = join_paths(funny_src_root_dir,'lib','public','linux32',src_engine_target)
	lib_folders = [
		'-L'+lib_folder,
		'-L'+join_paths(src_root_dir,'lib','public','linux32')
	]
	if get_option('funny')
		lib_folders += [
			'-L'+funny_lib_folder,
			'-L'+join_paths(funny_src_root_dir,'lib','public','linux32')
		]
	endif
elif target_system == 'windows'
	lib_folder = join_paths(src_root_dir,'lib','public','mingw32',src_engine_target)
	funny_lib_folder = join_paths(funny_src_root_dir,'lib','public','mingw32',src_engine_target)
	lib_folders = [
		'-L'+lib_folder,
		'-L'+join_paths(src_root_dir,'lib','public',src_engine_target),
		'-L'+join_paths(src_root_dir,'lib','public')
	]
	if get_option('funny')
		lib_folders += [
			'-L'+funny_lib_folder,
			'-L'+join_paths(funny_src_root_dir,'lib','public',src_engine_target),
			'-L'+join_paths(funny_src_root_dir,'lib','public')
		]
	endif
else
	error('unsupported system')
endif

lib_ext = '.a'

if target_system == 'linux' or target_system == 'gnu'
	sharedlib_ext = '.so'
	obj_ext = '.o'
elif target_system == 'windows'
	sharedlib_ext = '.dll'
	obj_ext = '.obj'
else
	error('unsupported system')
endif

funny_define = []
funny_incdir = []

if get_option('funny')
	funny_define = ['-DFUNNY']
	funny_incdir = [
		join_paths(funny_src_root_dir,'public'),
		join_paths(funny_src_root_dir,'common'),
	]
endif

engine_args = []

if src_engine_target == 'sdk2013mp'
	engine_args = [
		'-DHACKMGR_ENGINE_TARGET=HACKMGR_ENGINE_TARGET_SDK2013MP'
	]
elif src_engine_target == 'sdk2013sp'
	engine_args = [
		'-DHACKMGR_ENGINE_TARGET=HACKMGR_ENGINE_TARGET_SDK2013SP'
	]
else
	error('unknown engine target')
endif

source_dep = declare_dependency(
	compile_args: engine_args + funny_define + [
		'-DMOVE_CONSTRUCTOR_SUPPORT',
		'-DVALVE_RVALUE_REFS',
		'-DVALVE_INITIALIZER_LIST_SUPPORT',
		'-Wno-unknown-pragmas',
		'-Wno-unused-local-typedefs',
		'-Wno-template-id-cdtor',
		'-Wno-unused-value',
		'-Wno-sign-compare',
		'-Wno-reorder',
		#'-Wno-class-memaccess',
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
		#'-Wno-address',
		'-Wno-invalid-offsetof',
		'-Wno-conversion-null',
		'-Wno-switch',
		#'-Wno-uninitialized',
		'-Wno-logical-not-parentheses',
		'-Wno-int-in-bool-context',
		'-Wno-sequence-point',
		'-march=x86-64-v2',
		'-m32',
		'-mtune=generic',
		'-mmmx',
		'-msse',
		'-msse2',
		'-msse3',
		'-mssse3',
		'-msse4',
		'-msse4.1',
		'-msse4.2',
		'-mlzcnt',
		'-m3dnow',
		'-m3dnowa',
		'-fvisibility=hidden',
	],
	include_directories: funny_incdir + include_directories(
		src_root_dir,
		join_paths(src_root_dir,'public'),
		join_paths(src_root_dir,'common'),
		join_paths(src_root_dir,'utils','common')
	),
	link_args: lib_folders + [
		'-L'+join_paths(game_dir,'bin'),
		'-latomic'
	],
	dependencies: [
		source_debug_dep
	]
)

source_cl_dep = declare_dependency(
	link_args: [
		'-L'+join_paths(src_engine_dir,'bin'),
	],
)

source_ds_dep = declare_dependency(
	compile_args: [
		'-DSWDS',
		'-DDEDICATED',
	],
	link_args: [
		'-L'+join_paths(src_dedicated_engine_dir,'bin'),
	],
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
	sources: files(
		join_paths(common_dir,'module_name.cpp'),
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
	]
)

source_shared_lib_linux_dep = declare_dependency(
	compile_args: [
		'-shared',
	]
)

source_windows_dep = declare_dependency(
	compile_args: [
		'-DGNUC',
		'-DCOMPILER_GCC',
		'-DWIN32',
		'-D_WIN32',
		'-DIS_WINDOWS_PC',
		'-DPLATFORM_WINDOWS_PC',
		'-D_DLL_EXT=.dll',

		'-DNO_MALLOC_OVERRIDE',
	]
)

source_shared_lib_windows_dep = declare_dependency(
	compile_args: [
		'-shared','-mdll',
	]
)

memoverride_src = files(
	join_paths(public_dir,'tier0','memoverride.cpp'),
)

if target_system == 'linux' or target_system == 'gnu'
	os_dep = source_linux_dep
	source_shared_lib_os_dep = source_shared_lib_linux_dep
elif target_system == 'windows'
	os_dep = source_windows_dep
	source_shared_lib_os_dep = source_shared_lib_windows_dep
else
	error('unsupported system')
endif

source_shared_lib_dep_base = declare_dependency(
	dependencies: [
		source_dep
	],
	sources: memoverride_src + files(
		join_paths(common_dir,'module_name.cpp'),
		join_paths(common_dir,'debug_dll_check.cpp'),
	)
)

source_executable_dep_base = declare_dependency(
	dependencies: [
		source_dep
	],
	sources: memoverride_src + files(
		join_paths(common_dir,'module_name.cpp'),
		join_paths(common_dir,'debug_dll_check.cpp'),
	)
)

source_static_lib_dep = declare_dependency(
	dependencies: [
		source_static_lib_dep_base,
		os_dep
	],
)

source_shared_lib_dep = declare_dependency(
	compile_args: [
		'-shared',
	],
	dependencies: [
		source_shared_lib_dep_base,
		source_shared_lib_os_dep,
		os_dep
	],
)

source_executable_dep = declare_dependency(
	dependencies: [
		source_executable_dep_base,
		os_dep
	],
)
