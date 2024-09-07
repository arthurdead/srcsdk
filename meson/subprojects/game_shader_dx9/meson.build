project('game', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')

build_client = get_option('build_client')

stdshaders_src_dir = join_paths(src_root_dir,'materialsystem','stdshaders')
gameshaders_src_dir = join_paths(src_root_dir,'materialsystem','heistshaders')

game_shader_base = subproject('game_shader_base')

if build_client
	shared_library('game_shader_dx9',name_prefix:'',
		cpp_args: [
			'-DHEIST_DLL',
			'-DHEIST_SHADER_DLL',
		],
		sources: files(
			
		),
		include_directories: include_directories(
			
		),
		dependencies: [
			game_shader_base.get_variable('game_shader_base_dep')
		],
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)
endif
