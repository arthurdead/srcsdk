project('coplay')

src_root_dir = get_option('src_root_dir')

coplay_src_dir = join_paths(src_root_dir,'coplay','src')

coplay_dep = declare_dependency(
	compile_args: [
		#'-DCOPLAY_USE_LOBBIES',
		#'-DCOPLAY_DONT_SET_THREADMODE',
		'-DCOPLAY_DONT_UPDATE_RPC',
		'-DCOPLAY_DONT_LINK_SDL2',
		'-DCOPLAY_DONT_LINK_SDL2_NET'
	],
	sources: files(
		join_paths(coplay_src_dir,'coplay_connectionhandler.cpp'),
		join_paths(coplay_src_dir,'coplay_packetforward.cpp')
	),
	include_directories: include_directories(
		coplay_src_dir
	),
	dependencies: [
		dependency('SDL2_net')
	]
)
