project('theheist','cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

build_client = get_option('build_client')
build_listen_server = get_option('build_listen_server')
build_dedicated_server = get_option('build_dedicated_server')
build_libs = get_option('build_libs')

src_engine_dir = get_option('src_engine_dir')
game_dir = get_option('game_dir')

if meson.get_compiler('cpp').has_define('__WINE__')
	subproject('linux_hammer_ipc')
else
	if get_option('funny')
		subdir('funny')
	endif

	if build_libs
		subproject('choreoobjects')
		subproject('mathlib')
		subproject('tier1')
		subproject('raytrace')
		subproject('responserules')
		subproject('vgui_controls')
		subproject('hackmgr')
		subproject('shaderlib')
		subproject('recastnavigation_game')
		#subproject('vphysics')
		subproject('matchmaking')
		#subproject('shadercompiler')
	endif
	if build_client or build_listen_server
		subproject('game_loopback')
	endif
	if build_client
		#subproject('hammer_redirect')
		subproject('vaudio_minimp3')
		subproject('vaudio_redirect')
		#subproject('video_services')
		subproject('game_shader_dx9')
		subproject('gameui')
		subproject('gamepadui_redirect')
		subproject('serverbrowser_redirect')
		subproject('client')
	endif
	if build_listen_server
		subproject('server_listen')
	endif
	if build_dedicated_server
		#subproject('server_dedicated')
	endif
endif
