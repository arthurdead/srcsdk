project('shadercompiler', 'cpp', default_options: [
	'cpp_std=gnu++20'
])

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')
build_libs = get_option('build_libs')

source_base = subproject('source_base')

public_dir = join_paths(src_root_dir,'public')

shadercompiler_src_dir = join_paths(src_root_dir,'utils','shadercompiler')

if build_libs
	executable('shadercompiler',
		cpp_args: [
			'-DDLLNAME=shadercompiler',
		],
		win_subsystem: 'console',
		sources: files(
			join_paths(shadercompiler_src_dir,'main.cpp')
		),
		include_directories: include_directories(
			shadercompiler_src_dir,
		),
		dependencies: [
			source_base.get_variable('source_executable_dep'),
		],
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)
endif
