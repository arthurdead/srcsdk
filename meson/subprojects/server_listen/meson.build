project('game', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')
build_listen_server = get_option('build_listen_server')

server_src_dir = join_paths(src_root_dir,'game','server')

server = subproject('server')
server_listen_base = subproject('server_listen_base')

if build_listen_server
	shared_library('server',name_prefix:'',
		cpp_pch: [
			#join_paths(server_src_dir,'pch','cbase_pch.h'),
			#join_paths(server_src_dir,'pch','stdafx.cpp')
		],
		dependencies: [
			server.get_variable('server_dep'),
			server_listen_base.get_variable('server_listen_base_dep')
		],
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)
endif
