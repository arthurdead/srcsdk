project('server_dedicated', 'cpp', 'c', default_options: [
	'cpp_std=gnu++20'
])

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')
assert(meson.get_compiler('c').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')
build_dedicated_server = get_option('build_dedicated_server')

server_src_dir = join_paths(src_root_dir,'game','server')

server = subproject('server')
server_dedicated_base = subproject('server_dedicated_base')

if build_dedicated_server
	shared_library('server_srv',name_prefix:'',
		cpp_pch: [
			#join_paths(server_src_dir,'pch','cbase_pch.h'),
			#join_paths(server_src_dir,'pch','stdafx.cpp')
		],
		dependencies: [
			server.get_variable('server_dep'),
			server_dedicated_base.get_variable('server_dedicated_base_dep')
		],
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)
endif
