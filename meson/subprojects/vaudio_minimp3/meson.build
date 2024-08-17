project('vaudio_minimp3', 'cpp')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')

tier1 = subproject('tier1')
tier0 = subproject('tier0')

vaudio_minimp3_lib = shared_library('vaudio_minimp3',name_prefix:'',
	sources: files(
		join_paths(src_root_dir,'vaudio_minimp3','vaudio_minimp3.cpp')
	),
	include_directories: include_directories(
		join_paths(public_dir,'tier0'),
		join_paths(public_dir,'tier1'),
		join_paths(src_root_dir,'vaudio_minimp3')
	),
	dependencies: [
		tier1.get_variable('tier1_dep'),
		tier0.get_variable('tier0_dep'),
		source_base.get_variable('source_linux_shared_lib_dep')
	],
	install: true,
	install_dir: join_paths(game_dir,'bin')
)

vaudio_minimp3_dep = declare_dependency(
	link_with: vaudio_minimp3_lib
)
