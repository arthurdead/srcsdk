project('game_shader_dx9', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
game_dir = get_option('game_dir')

build_client = get_option('build_client')

stdshaders_src_dir = join_paths(src_root_dir,'materialsystem','stdshaders')
gameshaders_src_dir = join_paths(src_root_dir,'materialsystem','heistshaders')

game_shader_base = subproject('game_shader_base')

compileshader_script = join_paths(gameshaders_src_dir,'meson_compileshader.sh')

#template to compile a single shader
'''
shader = custom_target('shader',
	input: files(
		join_paths(gameshaders_src_dir,'shader.fxc')
	),
	command: [
		compileshader_script,
		'@INPUT@',
		gameshaders_src_dir,
		'@OUTDIR@'
	],
	output: [
		'@BASENAME@.inc',
		'@BASENAME@.vcs',
	],
	install: true,
	install_dir: [
		false,
		join_paths(game_dir,'game','shaders','fxc'),
	]
)
'''

if build_client
	shared_library('game_shader_dx9',name_prefix:'',
		cpp_args: [
			'-DHEIST_DLL',
			'-DHEIST_SHADER_DLL',
		],
		sources: [
			#add shaders targets here
		] + files(
			#add shader cpp sources here
		),
		include_directories: include_directories(
			stdshaders_src_dir
		),
		dependencies: [
			game_shader_base.get_variable('game_shader_base_dep')
		],
		install: true,
		install_dir: join_paths(game_dir,'bin'),
		install_rpath: '$ORIGIN'
	)
endif
