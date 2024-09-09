project('theheist','cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

build_client = get_option('build_client')
build_listen_server = get_option('build_listen_server')
build_dedicated_server = get_option('build_dedicated_server')
build_libs = get_option('build_libs')

if build_libs
	subproject('choreoobjects')
	subproject('mathlib')
	subproject('tier1')
	subproject('raytrace')
	subproject('responserules')
	subproject('vgui_controls')
	subproject('shaderlib')
	subproject('hackmgr')
	subproject('recastnavigation_game')
	subproject('game_loopback')
	#subproject('vphysics')
endif
if build_client
	subproject('vaudio_minimp3')
	#subproject('vaudio_redirect')
	#subproject('video_services')
	subproject('game_shader_dx9')
	subproject('client')
endif
if build_listen_server
	subproject('server_listen')
endif
if build_dedicated_server
	subproject('server_dedicated')
endif