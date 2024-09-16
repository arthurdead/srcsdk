project('shaderlib', 'cpp')

assert(meson.get_compiler('cpp').get_argument_syntax() == 'gcc', 'only gnu compilers are supported')

src_root_dir = get_option('src_root_dir')
build_libs = get_option('build_libs')

shaderlib_src_dir = join_paths(src_root_dir,'materialsystem','shaderlib')

hackmgr = subproject('hackmgr')

source_base = subproject('source_base')

if build_libs
	shaderlib_lib = static_library('shaderlib',name_prefix:'',
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
			source_base.get_variable('source_static_lib_dep')
		],
		install: true,
		install_dir: source_base.get_variable('lib_folder')
	)

	shaderlib_dep = declare_dependency(
		include_directories: include_directories(
			join_paths(src_root_dir,'public','shaderlib')
		),
		link_with: shaderlib_lib,
		dependencies: [
			hackmgr.get_variable('hackmgr_dep')
		]
	)
else
	shaderlib_dep = declare_dependency(
		include_directories: include_directories(
			join_paths(src_root_dir,'public','shaderlib')
		),
		link_args: [
			'-l:shaderlib'+source_base.get_variable('lib_ext')
		],
		dependencies: [
			hackmgr.get_variable('hackmgr_dep')
		]
	)
endif