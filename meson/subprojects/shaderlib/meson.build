project('shaderlib', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
shaderlib_src_dir = join_paths(src_root_dir,'materialsystem','shaderlib')

source_base = subproject('source_base')

shaderlib_lib = static_library('shaderlib',
	cpp_args: [
		'-DFAST_MATERIALVAR_ACCESS',
		'-DLIBNAME=shaderlib'
	],
	sources: files(
		join_paths(shaderlib_src_dir,'BaseShader.cpp'),
		join_paths(shaderlib_src_dir,'ShaderDLL.cpp'),
		join_paths(shaderlib_src_dir,'shaderlib_cvar.cpp')
	),
	include_directories: include_directories(
		join_paths(src_root_dir,'public','tier0'),
		join_paths(src_root_dir,'public','tier1'),
		join_paths(src_root_dir,'materialsystem')
	),
	dependencies: [
		source_base.get_variable('source_os_static_lib_dep')
	]
)

shaderlib_dep = declare_dependency(
	include_directories: include_directories(
		join_paths(src_root_dir,'public','shaderlib')
	),
	link_with: shaderlib_lib
)
