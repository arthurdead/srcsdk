project('hackmgr', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')
build_libs = get_option('build_libs')
build_dedicated_server = get_option('build_dedicated_server')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')
hackmgr_src_dir = join_paths(src_root_dir,'hackmgr')

tier2 = subproject('tier2')
tier1 = subproject('tier1')
tier0 = subproject('tier0')
vstdlib = subproject('vstdlib')
sdl2 = dependency('SDL2', required: false)
mathlib = subproject('mathlib')
steamworks = subproject('steamworks')

install_data(
	files(
		join_paths(hackmgr_src_dir,'hackmgr.vdf'),
	),
	install_mode: 'r--r--r--',
	install_dir: join_paths(game_dir,'bin')
)

hackmgr_base_dep = declare_dependency(
	sources: files(
		join_paths(public_dir,'hackmgr','depend_on_hackmgr.cpp'),
		join_paths(public_dir,'hackmgr','dlloverride.cpp'),
		join_paths(public_dir,'tier0','logging.cpp'),
	),
)

if build_libs
	hackmgr_base_lib = declare_dependency(
		compile_args: [
			'-DBUILDING_HACKMGR'
		],
		sources: files(
			join_paths(hackmgr_src_dir,'dlloverride.cpp'),
			join_paths(hackmgr_src_dir,'hackmgr.cpp'),
			join_paths(hackmgr_src_dir,'hackmgr_server.cpp'),
			join_paths(hackmgr_src_dir,'createinterface.cpp'),
			join_paths(hackmgr_src_dir,'isoundemittersystembase.cpp'),
			join_paths(hackmgr_src_dir,'IEngineTrace.cpp'),
			join_paths(hackmgr_src_dir,'ivmodelinfo.cpp'),
			join_paths(hackmgr_src_dir,'IStaticPropMgr.cpp'),
			join_paths(hackmgr_src_dir,'commandline.cpp'),
			join_paths(hackmgr_src_dir,'cvar.cpp'),
			join_paths(hackmgr_src_dir,'filesystem.cpp'),
			join_paths(hackmgr_src_dir,'dbg.cpp'),
			join_paths(hackmgr_src_dir,'cpumonitoring.cpp'),
			join_paths(hackmgr_src_dir,'memalloc.cpp'),
			join_paths(hackmgr_src_dir,'steam.cpp'),
			join_paths(hackmgr_src_dir,'ivdebugoverlay.cpp'),
			join_paths(src_root_dir,'tier0','mingw_linker_fixes.cpp'),
			join_paths(src_root_dir,'vstdlib','mingw_linker_fixes.cpp'),
			join_paths(public_dir,'SoundParametersInternal.cpp'),
			join_paths(public_dir,'interval.cpp'),
			join_paths(public_dir,'tier0','logging.cpp'),
			join_paths(public_dir,'dt_send.cpp'),
			join_paths(public_dir,'dt_recv.cpp'),
			join_paths(public_dir,'dt_common.cpp'),
		),
		include_directories: include_directories(
			hackmgr_src_dir,
			join_paths(public_dir,'tier0'),
			join_paths(public_dir,'tier1'),
			join_paths(public_dir,'hackmgr'),
			join_paths(src_root_dir,'materialsystem')
		),
		dependencies: [
			tier1.get_variable('tier1_dep'),
			tier2.get_variable('tier2_dep'),
			mathlib.get_variable('mathlib_dep'),
			source_base.get_variable('source_shared_lib_dep')
		],
	)

	hackmgr_lib = shared_library('hackmgr',name_prefix:'',
		cpp_args: [
			'-DDLLNAME=hackmgr',
			'-DMEMOVERRIDE_MODULE=hackmgr',
		],
		sources: [
			join_paths(hackmgr_src_dir,'istudiorender.cpp'),
			join_paths(hackmgr_src_dir,'hackmgr_client.cpp'),
			join_paths(hackmgr_src_dir,'ishadowmgr.cpp'),
			join_paths(hackmgr_src_dir,'ivmodelrender.cpp'),
			join_paths(hackmgr_src_dir,'shadersystem.cpp'),
			join_paths(hackmgr_src_dir,'toolframework.cpp'),
			join_paths(hackmgr_src_dir,'vgui.cpp'),
		],
		dependencies: [
			hackmgr_base_lib,
			sdl2,
			tier0.get_variable('tier0_dep'),
			vstdlib.get_variable('vstdlib_dep'),
			source_base.get_variable('source_cl_dep'),
			steamworks.get_variable('steamworks_dep'),
		],
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)

	hackmgr_dep = declare_dependency(
		dependencies: [
			hackmgr_base_dep,
		],
		link_with: hackmgr_lib
	)

	install_data(
		join_paths(src_root_dir,'tools','enginetools.txt'),
		install_dir: join_paths(game_dir,'bin'),
	)

	install_data(
		join_paths(src_root_dir,'tools','enginetools.txt'),
		rename: 'sdkenginetools.txt',
		install_dir: join_paths(game_dir,'bin'),
	)

	if build_dedicated_server
		hackmgr_ds_lib = shared_library('hackmgr_ds',name_prefix:'',
			cpp_args: [
				'-DDLLNAME=hackmgr_ds',
				'-DMEMOVERRIDE_MODULE=hackmgr_ds',
			],
			dependencies: [
				hackmgr_base_lib,
				source_base.get_variable('source_ds_dep'),
				tier0.get_variable('tier0_ds_dep'),
				vstdlib.get_variable('vstdlib_ds_dep'),
				steamworks.get_variable('steamworks_ds_dep'),
			],
			install: true,
			install_dir: join_paths(game_dir,'bin'),
			install_rpath: '$ORIGIN'
		)

		hackmgr_ds_dep = declare_dependency(
			dependencies: [
				hackmgr_base_dep,
			],
			link_with: hackmgr_ds_lib
		)
	endif
else
	hackmgr_dep = declare_dependency(
		dependencies: [
			hackmgr_base_dep,
		],
		link_args: '-l:hackmgr'+source_base.get_variable('sharedlib_ext')
	)

	hackmgr_ds_dep = declare_dependency(
		dependencies: [
			hackmgr_base_dep,
		],
		link_args: '-l:hackmgr_ds'+source_base.get_variable('sharedlib_ext')
	)
endif
