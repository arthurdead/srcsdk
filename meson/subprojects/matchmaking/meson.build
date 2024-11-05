project('matchmaking', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')
build_libs = get_option('build_libs')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')

tier1 = subproject('tier1')
tier0 = subproject('tier0')
hackmgr = subproject('hackmgr')

if build_libs
	matchmaking_base_lib = declare_dependency(
		compile_args: [
			'-DMATCHMAKING_DLL',
		],
		sources: files(
			join_paths(src_root_dir,'matchmaking','matchframework.cpp'),
			join_paths(src_root_dir,'matchmaking','matchext.cpp'),
		),
		include_directories: include_directories(
			join_paths(public_dir,'tier0'),
			join_paths(public_dir,'tier1'),
			join_paths(src_root_dir,'matchmaking')
		),
		dependencies: [
			tier1.get_variable('tier1_dep'),
			source_base.get_variable('source_shared_lib_dep')
		],
	)

	shared_library('matchmaking',name_prefix:'',
		cpp_args: [
			'-DDLLNAME=matchmaking',
			'-DMEMOVERRIDE_MODULE=matchmaking',
		],
		dependencies: [
			matchmaking_base_lib,
			hackmgr.get_variable('hackmgr_dep'),
			tier0.get_variable('tier0_dep'),
			source_base.get_variable('source_cl_dep')
		],
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)

	shared_library('matchmaking_ds',name_prefix:'',
		cpp_args: [
			'-DDLLNAME=matchmaking_ds',
			'-DMEMOVERRIDE_MODULE=matchmaking_ds',
			'-DMATCHMAKING_DS_DLL',
		],
		dependencies: [
			matchmaking_base_lib,
			hackmgr.get_variable('hackmgr_ds_dep'),
			tier0.get_variable('tier0_ds_dep'),
			source_base.get_variable('source_ds_dep'),
		],
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)
endif
