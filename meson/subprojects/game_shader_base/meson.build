project('game_shader_base','cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')

source_base = subproject('source_base')
tier1 = subproject('tier1')
tier0 = subproject('tier0')
vstdlib = subproject('vstdlib')
mathlib = subproject('mathlib')
shaderlib = subproject('shaderlib')

public_dir = join_paths(src_root_dir,'public')
common_dir = join_paths(src_root_dir,'common')

stdshaders_src_dir = join_paths(src_root_dir,'materialsystem','stdshaders')

game_shader_base_dep = declare_dependency(
	compile_args: [
		'-DDLLNAME=game_shader_dx9',
		'-DSTDSHADER_DX9_DLL_EXPORT',
		'-DFAST_MATERIALVAR_ACCESS',
		'-DGAME_SHADER_DLL'
	],
	sources: files(
		join_paths(stdshaders_src_dir,'BaseVSShader.cpp')
	),
	include_directories: include_directories(
		stdshaders_src_dir
	),
	dependencies: [
		source_base.get_variable('source_shared_lib_dep'),
		tier0.get_variable('tier0_dep'),
		tier1.get_variable('tier1_dep'),
		mathlib.get_variable('mathlib_dep'),
		vstdlib.get_variable('vstdlib_dep'),
		shaderlib.get_variable('shaderlib_dep')
	]
)
