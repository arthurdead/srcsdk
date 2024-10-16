project('vaudio_redirect', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')
build_client = get_option('build_client')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')

tier1 = subproject('tier1')
tier0 = subproject('tier0')
hackmgr = subproject('hackmgr')

vaudio_redirect_lib = declare_dependency(
	sources: files(
		join_paths(src_root_dir,'vaudio_redirect','vaudio_redirect.cpp')
	),
	include_directories: include_directories(
		join_paths(public_dir,'tier0'),
		join_paths(public_dir,'tier1'),
		join_paths(src_root_dir,'vaudio_redirect')
	),
	dependencies: [
		tier1.get_variable('tier1_dep'),
		tier0.get_variable('tier0_dep'),
		source_base.get_variable('source_shared_lib_dep')
	]
)

if build_client
	shared_library('vaudio_miles',name_prefix:'',
		cpp_args: [
			'-DDLLNAME=vaudio_miles',
			'-DMEMOVERRIDE_MODULE=vaudio_miles',
		],
		dependencies: [
			vaudio_redirect_lib,
			hackmgr.get_variable('hackmgr_dep'),
		],
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)
endif
