project('linux_hammer_ipc', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')
build_client = get_option('build_client')

if build_client
	linux_hammer_ipc_exe = executable('linux_hammer_ipc',name_suffix:'exe',
		win_subsystem: 'console',
		sources: files(
			join_paths(src_root_dir,'hammer_redirect','linux_hammer_ipc.cpp')
		),
		include_directories: include_directories(
			join_paths(src_root_dir,'hammer_redirect'),
			'/usr/include/wine/windows',
		),
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)

	custom_target(
		'linux_hammer_ipc_exe_so',
		depends: linux_hammer_ipc_exe,
		command: 'true',
		output: 'linux_hammer_ipc.exe.so',
		install: true,
		install_dir: join_paths(game_dir,'bin'),
	)
endif
