project('coplay','cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')

coplay_root_dir = join_paths(src_root_dir,'coplay')
coplay_src_dir = join_paths(coplay_root_dir,'src')

coplay_cpp_args = []

target_system = target_machine.system()
if target_system == 'linux' or target_system == 'gnu'
	coplay_lib_folder = join_paths(coplay_root_dir,'lib','public','linux32')
elif target_system == 'windows'
	coplay_lib_folder = join_paths(coplay_root_dir,'lib','public')
else
	error('unsupported system')
endif

sdl2_net = dependency('SDL2_net', required: false)
if not sdl2_net.found()
	sdl2_net = declare_dependency(
		include_directories: include_directories(
			join_paths(coplay_root_dir,'include')
		),
		link_args: [
			'-L'+coplay_lib_folder,
			'-lSDL2_net'
		]
	)
else
	coplay_cpp_args += [
		'-DCOPLAY_DONT_LINK_SDL2',
		'-DCOPLAY_DONT_LINK_SDL2_NET'
	]
endif

install_data(
	files(
		join_paths(coplay_root_dir,'lib','SDL2.dll'),
		join_paths(coplay_root_dir,'lib','SDL2_net.dll'),
		join_paths(coplay_root_dir,'lib','linux32','libSDL2.so'),
		join_paths(coplay_root_dir,'lib','linux32','libSDL2_net.so'),
	),
	install_mode: 'rwxr-xr-x',
	install_dir: join_paths(game_dir,'bin')
)

coplay_dep = declare_dependency(
	compile_args: [
		#'-DCOPLAY_USE_LOBBIES',
		#'-DCOPLAY_DONT_SET_THREADMODE',
		'-DCOPLAY_DONT_UPDATE_RPC'
	] + coplay_cpp_args,
	sources: files(
		join_paths(coplay_src_dir,'coplay_connectionhandler.cpp'),
		join_paths(coplay_src_dir,'coplay_packetforward.cpp')
	),
	include_directories: include_directories(
		coplay_src_dir
	),
	dependencies: [
		sdl2_net
	]
)