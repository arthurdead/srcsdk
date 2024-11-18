project('client', 'cpp', 'c')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')
assert(meson.get_compiler('c').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')
build_client = get_option('build_client')

client_base = subproject('client_base')
game_shared = subproject('game_shared')
coplay = subproject('coplay')

client_src_dir = join_paths(src_root_dir,'game','client')
client_game_src_dir = join_paths(client_src_dir,'heist')
client_game_vgui_src_dir = join_paths(client_game_src_dir,'vgui')
client_game_hud_src_dir = join_paths(client_game_src_dir,'hud')
client_game_npcs_src_dir = join_paths(client_game_src_dir,'npcs')
client_game_entities_src_dir = join_paths(client_game_src_dir,'entities')

if build_client
	shared_library('client',name_prefix:'',
		cpp_args: [
			'-DHEIST_CLIENT_DLL',
		],
		cpp_pch: [
			#join_paths(client_src_dir,'pch','cbase_pch.h'),
			#join_paths(client_src_dir,'pch','stdafx.cpp')
		],
		sources: files(
			join_paths(client_game_src_dir,'c_heist_player.cpp'),
			join_paths(client_game_hud_src_dir,'hud_suspicion.cpp'),
			join_paths(client_game_npcs_src_dir,'c_npc_humanoidbase.cpp'),
			join_paths(client_game_src_dir,'heist_clientmode.cpp'),
			join_paths(client_game_src_dir,'in_heist.cpp'),
		),
		include_directories: include_directories(
			client_game_src_dir,
			client_game_vgui_src_dir,
			client_game_entities_src_dir,
			client_game_hud_src_dir
		),
		dependencies: [
			client_base.get_variable('client_base_dep'),
			game_shared.get_variable('game_shared_dep'),
			#coplay.get_variable('coplay_dep')
		],
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)
endif
