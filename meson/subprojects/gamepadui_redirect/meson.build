project('gamepadui_redirect', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')
build_client = get_option('build_client')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')

tier1 = subproject('tier1')
tier0 = subproject('tier0')
tier2 = subproject('tier2')
tier3 = subproject('tier3')
hackmgr = subproject('hackmgr')

if build_client
	shared_library('gamepadui',name_prefix:'',
		cpp_args: [
			'-DDLLNAME=gamepadui',
			'-DMEMOVERRIDE_MODULE=gamepadui',
			'-DGAMEPADUI_DLL',
		],
		sources: files(
			join_paths(src_root_dir,'gamepadui_redirect','gamepadui_redirect.cpp')
		),
		include_directories: include_directories(
			join_paths(public_dir,'tier0'),
			join_paths(public_dir,'tier1'),
			join_paths(src_root_dir,'gamepadui_redirect')
		),
		dependencies: [
			hackmgr.get_variable('hackmgr_dep'),
			tier1.get_variable('tier1_dep'),
			tier0.get_variable('tier0_dep'),
			tier2.get_variable('tier2_dep'),
			tier3.get_variable('tier3_dep'),
			source_base.get_variable('source_shared_lib_dep'),
			source_base.get_variable('source_cl_dep'),
		],
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)
endif
