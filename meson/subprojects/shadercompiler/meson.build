project('shadercompiler', 'cpp', default_options: [
	'cpp_std=gnu++20'
])

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')
build_libs = get_option('build_libs')

shadercompiler_src_dir = join_paths(src_root_dir,'utils','shadercompiler')

target_system = target_machine.system()
assert(target_system == 'linux' or target_system == 'gnu')

if build_libs
	m4  = find_program('m4', native: true)

	do_passes_h = custom_target('do_passes.h',
		input: files(
			join_paths(shadercompiler_src_dir,'do_passes.m4')
		),
		output : 'do_passes.h',
		command : [m4,'-E','-P','@INPUT@'],
		capture: true,
	)

	executable('shadercompiler',
		win_subsystem: 'console',
		sources: [do_passes_h] + files(
			join_paths(shadercompiler_src_dir,'main.cpp')
		),
		include_directories: include_directories(
			shadercompiler_src_dir,
		),
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)
endif
