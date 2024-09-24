project('hackmgr', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')
build_libs = get_option('build_libs')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')
hackmgr_src_dir = join_paths(src_root_dir,'hackmgr')

tier1 = subproject('tier1')
tier0 = subproject('tier0')
vstdlib = subproject('vstdlib')

install_data(
	files(
		join_paths(hackmgr_src_dir,'hackmgr.vdf'),
	),
	install_mode: 'r--r--r--',
	install_dir: join_paths(game_dir,'bin')
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
			join_paths(hackmgr_src_dir,'ivdebugoverlay.cpp'),
			join_paths(hackmgr_src_dir,'isoundemittersystembase.cpp'),
			join_paths(hackmgr_src_dir,'IEngineTrace.cpp'),
			join_paths(hackmgr_src_dir,'ivmodelinfo.cpp'),
			join_paths(hackmgr_src_dir,'IStaticPropMgr.cpp'),
			join_paths(hackmgr_src_dir,'commandline.cpp'),
			join_paths(hackmgr_src_dir,'cvar.cpp'),
			join_paths(hackmgr_src_dir,'filesystem.cpp'),
			join_paths(hackmgr_src_dir,'dbg.cpp'),
			join_paths(hackmgr_src_dir,'cpumonitoring.cpp'),
			join_paths(public_dir,'SoundParametersInternal.cpp'),
			join_paths(public_dir,'interval.cpp'),
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
			tier0.get_variable('tier0_dep'),
			vstdlib.get_variable('vstdlib_dep'),
			source_base.get_variable('source_shared_lib_dep')
		],
	)

	hackmgr_lib = shared_library('hackmgr',name_prefix:'',
		cpp_args: [
			'-DDLLNAME=hackmgr',
		],
		sources: [
			join_paths(hackmgr_src_dir,'istudiorender.cpp'),
			join_paths(hackmgr_src_dir,'hackmgr_client.cpp'),
			join_paths(hackmgr_src_dir,'ishadowmgr.cpp'),
			join_paths(hackmgr_src_dir,'ivmodelrender.cpp'),
			join_paths(hackmgr_src_dir,'shadersystem.cpp'),
			join_paths(hackmgr_src_dir,'toolframework.cpp'),
		],
		dependencies: [
			hackmgr_base_lib,
		],
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)

	hackmgr_sv_lib = shared_library('hackmgr_sv',name_prefix:'',
		cpp_args: [
			'-DDLLNAME=hackmgr_sv',
			'-DDEDICATED',
			'-DSWDS',
		],
		dependencies: [
			hackmgr_base_lib,
		],
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)

	hackmgr_base_dep = declare_dependency(
		sources: files(
			join_paths(public_dir,'hackmgr','depend_on_hackmgr.cpp')
		),
	)

	hackmgr_dep = declare_dependency(
		dependencies: [
			hackmgr_base_dep,
		],
		link_with: hackmgr_lib
	)

	hackmgr_sv_dep = declare_dependency(
		dependencies: [
			hackmgr_base_dep,
		],
		link_with: hackmgr_sv_lib
	)
else
	hackmgr_base_dep = declare_dependency(
		sources: files(
			join_paths(public_dir,'hackmgr','depend_on_hackmgr.cpp')
		),
	)

	hackmgr_dep = declare_dependency(
		dependencies: [
			hackmgr_base_dep,
		],
		link_args: '-l:hackmgr'+source_base.get_variable('sharedlib_ext')
	)

	hackmgr_sv_dep = declare_dependency(
		dependencies: [
			hackmgr_base_dep,
		],
		link_args: '-l:hackmgr_sv'+source_base.get_variable('sharedlib_ext')
	)
endif
