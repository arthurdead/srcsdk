project('game_switcher', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')
assert(meson.get_compiler('c').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')

server_src_dir = join_paths(src_root_dir,'game','server')
client_src_dir = join_paths(src_root_dir,'game','client')

shared_library('client',name_prefix:'',
	sources: files(
		join_paths(server_src_dir,'server_switcher.cpp'),
	),
	install: true,
	install_dir: join_paths(game_dir,'bin'),
	install_rpath: '$ORIGIN'
)

shared_library('server',name_prefix:'',
	sources: files(
		join_paths(client_src_dir,'client_switcher.cpp'),
	),
	install: true,
	install_dir: join_paths(game_dir,'bin'),
	install_rpath: '$ORIGIN'
)
