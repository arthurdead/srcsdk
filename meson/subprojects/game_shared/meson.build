project('game', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')

shared_game_src_dir = join_paths(src_root_dir,'game','shared','heist')
shared_game_weapons_src_dir = join_paths(shared_game_src_dir,'weapons')
shared_game_npcs_src_dir = join_paths(shared_game_src_dir,'npcs')

game_shared_base = subproject('game_shared_base')

game_shared_dep = declare_dependency(
	compile_args: [
		'-DHEIST_DLL'
	],
	sources: files(
		join_paths(shared_game_src_dir,'heist_gamerules.cpp'),
		join_paths(shared_game_src_dir,'heist_player_shared.cpp'),
		join_paths(shared_game_src_dir,'heist_ammodef.cpp'),
		join_paths(shared_game_npcs_src_dir,'suspicioner.cpp'),
	),
	include_directories: include_directories(
		shared_game_src_dir,
		shared_game_weapons_src_dir,
		shared_game_npcs_src_dir
	),
	dependencies: [
		game_shared_base.get_variable('game_shared_base_dep')
	],
)
