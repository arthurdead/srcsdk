project('game', 'cpp')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')

client_src_dir = join_paths(src_root_dir,'game','client')
client_game_src_dir = join_paths(client_src_dir,'heist')
client_game_vgui_src_dir = join_paths(client_game_src_dir,'vgui')
client_game_hud_src_dir = join_paths(client_game_src_dir,'hud')
client_game_npcs_src_dir = join_paths(client_game_src_dir,'npcs')
client_game_entities_src_dir = join_paths(client_game_src_dir,'entities')

server_src_dir = join_paths(src_root_dir,'game','server')
server_game_src_dir = join_paths(server_src_dir,'heist')
server_game_npcs_src_dir = join_paths(server_game_src_dir,'npcs')
server_game_entities_src_dir = join_paths(server_game_src_dir,'entities')

shared_game_src_dir = join_paths(src_root_dir,'game','shared','heist')
shared_game_weapons_src_dir = join_paths(shared_game_src_dir,'weapons')
shared_game_npcs_src_dir = join_paths(shared_game_src_dir,'npcs')

source_game_base = subproject('source_game_base')

stdshaders_src_dir = join_paths(src_root_dir,'materialsystem','stdshaders')
gameshaders_src_dir = join_paths(src_root_dir,'materialsystem','heistshaders')

coplay = subproject('coplay')

game_shared_dep = declare_dependency(
	compile_args: [
		'-DHEIST_DLL',
		'-DSecobMod__SAVERESTORE',
		'-DSecobMod__MULTIPLAYER_LEVEL_TRANSITIONS',
		'-DSecobMod__MULTIPLAYER_VIEWCONTROL_CAMERAS',
		'-DSecobMod__ENABLE_DYNAMIC_PLAYER_RESPAWN_CODE',
		'-DSecobMod__ENABLE_FAKE_PASSENGER_SEATS',
		'-DSecobMod__ALLOW_PLAYER_MODELS_IN_VEHICLES',
		'-DSM_SP_FIXES',
		'-DSM_AI_FIXES',
		'-DSM_IMPROVEMENTS'
	],
	sources: files(
		join_paths(shared_game_src_dir,'heist_gamerules.cpp'),
		join_paths(shared_game_src_dir,'heist_playeranimstate.cpp'),
		join_paths(shared_game_weapons_src_dir,'weapon_heistbase.cpp'),
		join_paths(shared_game_src_dir,'heist_usermessages.cpp'),
		join_paths(shared_game_src_dir,'heist_gamemovement.cpp'),
		join_paths(shared_game_weapons_src_dir,'heist_weapon_parse.cpp'),
		join_paths(shared_game_src_dir,'heist_player_shared.cpp'),
		join_paths(shared_game_npcs_src_dir,'suspicioner.cpp'),
	),
	include_directories: include_directories(
		shared_game_src_dir,
		shared_game_weapons_src_dir,
		shared_game_npcs_src_dir
	)
)

server_lib_dep = declare_dependency(
	compile_args: [
		'-DHEIST_GAME_DLL'
	],
	sources: files(
		join_paths(server_game_src_dir,'heist_gameinterface.cpp'),
		join_paths(server_game_src_dir,'heist_client.cpp'),
		join_paths(server_game_src_dir,'heist_player.cpp'),
		join_paths(server_game_src_dir,'heist_playermove.cpp'),
		join_paths(server_game_src_dir,'heist_eventlog.cpp'),
		join_paths(server_game_entities_src_dir,'vgui_text_display.cpp'),
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
		game_shared_dep
	]
)

shared_library('server',name_prefix:'',
	cpp_pch: [
		#join_paths(server_src_dir,'pch','cbase_pch.h'),
		#join_paths(server_src_dir,'pch','stdafx.cpp')
	],
	dependencies: [
		server_lib_dep,
		source_game_base.get_variable('source_linux_server_lib_dep')
	],
	install: true,
	install_dir: join_paths(game_dir,'bin')
)

if false
shared_library('server_srv',name_prefix:'',
	cpp_pch: [
		#join_paths(server_src_dir,'pch','cbase_pch.h'),
		#join_paths(server_src_dir,'pch','stdafx.cpp')
	],
	dependencies: [
		server_lib_dep,
		source_game_base.get_variable('source_linux_dedicated_server_lib_dep')
	],
	install: true,
	install_dir: join_paths(game_dir,'bin')
)
endif

shared_library('client',name_prefix:'',
	cpp_args: [
		'-DHEIST_CLIENT_DLL',
		'-DWEBM_VIDEO_SERVICES_MOD'
	],
	cpp_pch: [
		#join_paths(client_src_dir,'pch','cbase_pch.h'),
		#join_paths(client_src_dir,'pch','stdafx.cpp')
	],
	sources: files(
		join_paths(client_game_src_dir,'c_heist_player.cpp'),
		join_paths(client_game_src_dir,'heist_in_main.cpp'),
		join_paths(client_game_src_dir,'heist_prediction.cpp'),
		join_paths(client_game_src_dir,'clientmode_heist.cpp'),
		join_paths(client_game_hud_src_dir,'heist_hud_chat.cpp'),
		join_paths(client_game_hud_src_dir,'heist_hud_weaponselection.cpp'),
		join_paths(client_game_hud_src_dir,'hud_suspicion.cpp'),
		join_paths(client_game_npcs_src_dir,'c_npc_humanoidbase.cpp'),
		join_paths(client_game_entities_src_dir,'c_vgui_text_display.cpp'),
		join_paths(client_game_vgui_src_dir,'heistviewport.cpp'),
		join_paths(client_game_vgui_src_dir,'vgui_rootpanel_heist.cpp'),
	),
	include_directories: include_directories(
		client_game_src_dir,
		client_game_vgui_src_dir,
		client_game_entities_src_dir,
		client_game_hud_src_dir
	),
	dependencies: [
		game_shared_dep,
		source_game_base.get_variable('source_linux_client_lib_dep'),
		coplay.get_variable('coplay_dep')
	],
	install: true,
	install_dir: join_paths(game_dir,'bin')
)

shared_library('game_shader_dx9',name_prefix:'',
	cpp_args: [
		'-DHEIST_SHADER_DLL',
	],
	sources: files(
		
	),
	include_directories: include_directories(
		
	),
	dependencies: [
		source_game_base.get_variable('source_linux_game_shader_lib_dep')
	],
	install: true,
	install_dir: join_paths(game_dir,'bin')
)
