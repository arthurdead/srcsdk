project('game', 'cpp')

src_root_dir = get_option('src_root_dir')

client_game_src_dir = join_paths(src_root_dir,'game','client','heist')
client_game_vgui_src_dir = join_paths(src_root_dir,'game','client','heist','vgui')
server_game_src_dir = join_paths(src_root_dir,'game','server','heist')
shared_game_src_dir = join_paths(src_root_dir,'game','shared','heist')

source_game_base = subproject('source_game_base')

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
		join_paths(shared_game_src_dir,'weapon_heistbase.cpp'),
		join_paths(shared_game_src_dir,'heist_usermessages.cpp'),
		join_paths(shared_game_src_dir,'heist_gamemovement.cpp'),
		join_paths(shared_game_src_dir,'heist_weapon_parse.cpp'),
		join_paths(shared_game_src_dir,'heist_player_shared.cpp'),
	),
	include_directories: include_directories(
		shared_game_src_dir
	)
)

shared_library('server',
	cpp_args: [
		'-DHEIST_GAME_DLL'
	],
	cpp_pch: [
		#join_paths(server_src_dir,'cbase.h'),
		#join_paths(server_src_dir,'stdafx.cpp')
	],
	sources: files(
		join_paths(server_game_src_dir,'heist_gameinterface.cpp'),
		join_paths(server_game_src_dir,'heist_client.cpp'),
		join_paths(server_game_src_dir,'heist_player.cpp'),
		join_paths(server_game_src_dir,'heist_playermove.cpp'),
		join_paths(server_game_src_dir,'heist_eventlog.cpp'),
		join_paths(server_game_src_dir,'monster_dummy.cpp')
	),
	include_directories: include_directories(
		server_game_src_dir
	),
	dependencies: [
		game_shared_dep,
		source_game_base.get_variable('source_linux_server_lib_dep')
	]
)

shared_library('client',
	cpp_args: [
		'-DHEIST_CLIENT_DLL',
		'-DWEBM_VIDEO_SERVICES_MOD'
	],
	cpp_pch: [
		#join_paths(client_src_dir,'cbase.h'),
		#join_paths(client_src_dir,'stdafx.cpp')
	],
	sources: files(
		join_paths(client_game_src_dir,'c_heist_player.cpp'),
		join_paths(client_game_src_dir,'heist_in_main.cpp'),
		join_paths(client_game_src_dir,'heist_prediction.cpp'),
		join_paths(client_game_src_dir,'clientmode_heist.cpp'),
		join_paths(client_game_src_dir,'heist_hud_chat.cpp'),
		join_paths(client_game_src_dir,'heist_hud_weaponselection.cpp'),
		join_paths(client_game_vgui_src_dir,'heistviewport.cpp'),
		join_paths(client_game_vgui_src_dir,'vgui_rootpanel_heist.cpp'),
	),
	include_directories: include_directories(
		client_game_src_dir,
		client_game_vgui_src_dir
	),
	dependencies: [
		game_shared_dep,
		source_game_base.get_variable('source_linux_client_lib_dep'),
		coplay.get_variable('coplay_dep')
	]
)
