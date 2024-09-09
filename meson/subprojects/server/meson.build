project('server', 'cpp', 'c')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')
assert(meson.get_compiler('c').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')

server_src_dir = join_paths(src_root_dir,'game','server')
server_game_src_dir = join_paths(server_src_dir,'heist')
server_game_npcs_src_dir = join_paths(server_game_src_dir,'npcs')
server_game_entities_src_dir = join_paths(server_game_src_dir,'entities')

server_base = subproject('server_base')
game_shared = subproject('game_shared')

server_dep = declare_dependency(
	compile_args: [
		'-DHEIST_GAME_DLL'
	],
	sources: files(
		join_paths(server_game_src_dir,'heist_gameinterface.cpp'),
		join_paths(server_game_src_dir,'heist_client.cpp'),
		join_paths(server_game_src_dir,'heist_player.cpp'),
		join_paths(server_game_npcs_src_dir,'npc_dev.cpp'),
		join_paths(server_game_npcs_src_dir,'npc_humanoidbase.cpp'),
		join_paths(server_game_npcs_src_dir,'npc_cop.cpp'),
		join_paths(server_game_npcs_src_dir,'npc_civilian.cpp')
	),
	include_directories: include_directories(
		server_game_src_dir,
		server_game_npcs_src_dir,
		server_game_entities_src_dir
	),
	dependencies: [
		server_base.get_variable('server_base_dep'),
		game_shared.get_variable('game_shared_dep')
	]
)
