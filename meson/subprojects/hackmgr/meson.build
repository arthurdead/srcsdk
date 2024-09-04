project('hackmgr', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')
hackmgr_src_dir = join_paths(src_root_dir,'hackmgr')

tier1 = subproject('tier1')
tier0 = subproject('tier0')

hackmgr_lib = shared_library('hackmgr',name_prefix:'',
	cpp_args: [
		'-DDLLNAME=hackmgr',
		'-DBUILDING_HACKMGR'
	],
	sources: files(
		join_paths(hackmgr_src_dir,'ishadowmgr.cpp'),
		join_paths(hackmgr_src_dir,'dlloverride.cpp'),
		join_paths(hackmgr_src_dir,'hackmgr.cpp'),
		join_paths(hackmgr_src_dir,'engine_interface.cpp'),
	),
	include_directories: include_directories(
		hackmgr_src_dir,
		join_paths(public_dir,'tier0'),
		join_paths(public_dir,'tier1'),
		join_paths(public_dir,'hackmgr')
	),
	dependencies: [
		tier1.get_variable('tier1_dep'),
		tier0.get_variable('tier0_dep'),
		source_base.get_variable('source_os_shared_lib_dep')
	],
	install: true,
	install_dir: join_paths(game_dir,'bin')
)

hackmgr_dep = declare_dependency(
	sources: files(
		join_paths(public_dir,'hackmgr','depend_on_hackmgr.cpp')
	),
	link_with: hackmgr_lib
)
